#include <iostream>   // std::cout, std::cin, std::endl
#include <unistd.h>   // fork(), execve(), access() -- chamadas POSIX do Unix
#include <sys/wait.h> // para waitpid()
#include <string>
#include <vector>
#include <deque>
#include <cstdlib> // getenv()
#include <sstream>
#include <climits> 


extern char** environ; //Isso aponta para uma variável que já existe globalmente no sistema
                       //ex de variavel: "NOME=valor". Usaremos ao chamarmos execve.

void process_command(const std::string& line); // evitar erro de execucao, pois metodo era declarado e criado dps de outro metodo que o chamava

std::string get_home(){
    const char* home = getenv("HOME");
    if (home == nullptr){
        return "";
    }
    return std::string(home);
}

namespace Session {//package para guardar historico evitando conflito
    std::deque<std::string> history; // usado deck para facilitar adicao e remocao


    void add(const std::string& command) {
        history.push_front(command);      // comando novo sempre entra na frente (indice 0)
        if (history.size() > 10) {
            history.pop_back();           // se passar de 10, tira o mais antigo do fim
        }
    }

    void print() {
        // imprime do mais antigo (indice maior) pro mais recente (indice 0)
        for (int i = (int)history.size() - 1; i >= 0; i--) {
            std::cout << i << " " << history[i] << std::endl;
        }
    }

}

std::string get_prompt() {
    const char * ps1 = getenv("PS1");
    if (ps1 != nullptr){
        return std::string(ps1);
    }
    return "$";
}

std::vector<std::string> parse_command(const std::string& line) {
    std::vector<std::string> args;

    size_t inicio = 0; // size_t eh um inteiro nao negativo, usado para lidar com strings
    size_t fim = line.find(' ');       // onde esta o proximo espaco (ou npos se nao achar)

    while (fim != std::string::npos) { // npos = posicao nao encontrada numa string
        if (fim > inicio) {
            // so adiciona se o pedaco nao for vazio (protege contra espacos repetidos, ex: "ls   -la")
            args.push_back(line.substr(inicio, fim - inicio));
            //substr -> a partir do indice x, corte y posicoes e guarde em line
        }
        inicio = fim + 1;              // pula o espaco que acabamos de achar
        fim = line.find(' ', inicio);  // procura o PROXIMO espaco, a partir daqui
    }

    // depois do loop, ainda sobra o ULTIMO pedaco (depois do ultimo espaco, ate o fim da linha)
    if (inicio < line.length()) {
        args.push_back(line.substr(inicio)); //1 parametro, corta da posicao ate o fim
    }

    return args;
}

std::string find_in_path(const std::string& command){

    //se tem /, nao procura em PATH
    if (command.find('/') != std::string::npos){
        if (access(command.c_str(), X_OK) == 0) {
            return command;
        }
        return "";
    }

    const char* path_env = getenv("PATH"); //pega o endereco de PATH e le
    if (path_env ==nullptr){
        return ""; // trata caso PATH nao exista
    }

    std::string path_str(path_env);
    std::istringstream path_stream(path_str);
    std::string dir;

    //ex PATH = "/usr/local/bin:/usr/bin:/bin" -- separado por ':'
    // getline com terceiro parametro ':' quebra a string quando houver ':'
    while (std::getline(path_stream,dir,':')){
        std::string full_path = dir + "/" +command;
        if (access(full_path.c_str(), X_OK) == 0){
            return full_path;
        }
    }

    return "";
}

void execute_external_command(const std::string& path, std::vector<std::string>& args){

    std::vector<char*> argv;

    for (auto& tok : args) {
        argv.push_back(const_cast<char*>(tok.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();

    if (pid < 0) {
        std::cout << "Erro de execucao!" << std::endl;
        return;
    }else if (pid == 0) {
        execve(path.c_str(), argv.data(), environ);

        std::cout << "Erro ao executar: " << args[0] << std::endl;
        exit(1);
    } else {
        waitpid(pid, nullptr, 0);
    }
}

bool execute_internal_command(const std::vector<std::string>& args) {
    const std::string& command = args[0];

    if (command == "exit") {
        int code = 0;

        if (args.size() > 1){
            try{
                code = std::stoi(args[1]);
            
            }catch(...){
                code = 0;
            }
        }
        exit(code);
    }

    if (command == "pwd") {
        char buffer[PATH_MAX];
        if (getcwd(buffer, sizeof(buffer)) != nullptr){
            std::cout << buffer << std::endl;
        }else {
            std::cout << "pwd: erro ao obter dir atual" << std::endl;
        }
        return true;
    }

    if (command == "cd"){
        std::string destination;
        if (args.size() > 1) {
            destination = args[1];
        }
        else {
            destination = get_home();
        }
        if (chdir(destination.c_str()) != 0){
            std::cout << "cd: dir nao encontrado: " << destination << std::endl;
        }
        return true;
    }

    if (command == "history") {

        if (args.size() == 1) {
            // "history" sem argumentos 
            Session::print();
            return true;
        }

        if (args[1] == "-c") {
            // "history -c" -- apaga tudo
            Session::history.clear();
            return true;
        }

        // "history N" 
        try {
            int offset = std::stoi(args[1]);
            if (offset < 0 || offset >= (int)Session::history.size()) {
                std::cout << "history: offset invalido: " << args[1] << std::endl;
                return true;
            }
            std::string saved_command = Session::history[offset];
            std::cout << saved_command << std::endl;
            process_command(saved_command); 
        } catch (...) {
            std::cout << "history: argumento invalido: " << args[1] << std::endl;
        }
        return true;
    }
    
    return false;
}

void process_command(const std::string& line) {
    std::vector<std::string> args = parse_command(line);

    if (args.empty()){
        return;
    }

    if (execute_internal_command(args)){
        return;
    }

    std::string command = args[0];

    std::string path = find_in_path(command);

    if (path.empty()){
        std::cout << "Command not found: " << command << std::endl;
        return ;
    }

    execute_external_command(path, args);
}



int main() {
    while (true) {
        std::cout << get_prompt();
 
        std::string line;
        if (!std::getline(std::cin, line)) break; // Ctrl+D encerra a shell sem travar
 
        process_command(line);

        if (!line.empty()) {
            Session::add(line);
        }
    }
    return 0;
}
