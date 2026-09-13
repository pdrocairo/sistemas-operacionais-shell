#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <climits>   // PATH_MAX, usado no buffer do getcwd

extern char** environ;

namespace Sessao {
    std::deque<std::string> historico;
}

std::string get_home() {
    const char* home = getenv("HOME");
    if (home == nullptr) {
        return "";
    }
    return std::string(home);
}

std::string get_prompt() {
    const char* ps1 = getenv("PS1");
    if (ps1 != nullptr) {
        return std::string(ps1);
    }
    return "$";
}

std::vector<std::string> parse_command(const std::string& line) {
    std::vector<std::string> tokens;
    size_t inicio = 0;
    size_t fim = line.find(' ');

    while (fim != std::string::npos) {
        if (fim > inicio) {
            tokens.push_back(line.substr(inicio, fim - inicio));
        }
        inicio = fim + 1;
        fim = line.find(' ', inicio);
    }
    if (inicio < line.length()) {
        tokens.push_back(line.substr(inicio));
    }
    return tokens;
}

std::string find_in_path(const std::string& command) {
    if (command.find('/') != std::string::npos) {
        if (access(command.c_str(), X_OK) == 0) {
            return command;
        }
        return "";
    }

    const char* path_env = getenv("PATH");
    if (path_env == nullptr) {
        return "";
    }

    std::string path_str(path_env);
    std::istringstream path_stream(path_str);
    std::string dir;

    while (std::getline(path_stream, dir, ':')) {
        std::string caminho_completo = dir + "/" + command;
        if (access(caminho_completo.c_str(), X_OK) == 0) {
            return caminho_completo;
        }
    }

    return "";
}

void executar_comando_externo(const std::string& caminho, std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (auto& tok : args) {
        argv.push_back(const_cast<char*>(tok.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();

    if (pid < 0) {
        std::cout << "Erro de execução!" << std::endl;
        return;
    } else if (pid == 0) {
        execve(caminho.c_str(), argv.data(), environ);
        std::cout << "Erro ao executar: " << args[0] << std::endl;
        exit(1);
    } else {
        waitpid(pid, nullptr, 0);
    }
}

// ------------------------------------------------------------------
// PASSO 6: comandos internos -- exit, pwd, cd.
// Devolve true se o comando foi tratado aqui (era um builtin),
// devolve false se nao era nenhum builtin conhecido (segue pra busca externa).
// ------------------------------------------------------------------
bool executar_comando_interno(const std::vector<std::string>& tokens) {
    const std::string& comando = tokens[0];

    if (comando == "exit") {
        int codigo = 0; // codigo de saida padrao, se o usuario nao especificar
        if (tokens.size() > 1) {
            try {
                codigo = std::stoi(tokens[1]); // converte a string do argumento pra int
            } catch (...) {
                codigo = 0; // se o argumento nao for um numero valido, usa 0 mesmo
            }
        }
        exit(codigo);
    }

    if (comando == "pwd") {
        char buffer[PATH_MAX];
        if (getcwd(buffer, sizeof(buffer)) != nullptr) {
            std::cout << buffer << std::endl;
        } else {
            std::cout << "pwd: erro ao obter diretório atual" << std::endl;
        }
        return true;
    }

    if (comando == "cd") {
        std::string destino;
        if (tokens.size() > 1) {
            destino = tokens[1]; // "cd algumapasta"
        } else {
            destino = get_home(); // "cd" sozinho vai pro HOME, igual bash de verdade
        }

        if (chdir(destino.c_str()) != 0) {
            std::cout << "cd: diretório não encontrado: " << destino << std::endl;
        }
        return true;
    }

    return false; // nao e nenhum builtin conhecido
}

void process_command(const std::string& line) {
    std::vector<std::string> tokens = parse_command(line);
    if (tokens.empty()) return;

    // Primeiro tenta como comando interno. So se NAO for, tenta externo.
    if (executar_comando_interno(tokens)) {
        return;
    }

    std::string comando = tokens[0];
    std::string caminho = find_in_path(comando);

    if (caminho.empty()) {
        std::cout << "Command not found: " << comando << std::endl;
        return;
    }

    executar_comando_externo(caminho, tokens);
}

int main() {
    while (true) {
        std::cout << get_prompt();

        std::string line;
        if (!std::getline(std::cin, line)) break;

        process_command(line);
    }
    return 0;
}