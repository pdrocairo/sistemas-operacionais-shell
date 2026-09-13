#include <iostream>   // std::cout, std::cin, std::endl
#include <unistd.h>   // fork(), execve(), access() -- chamadas POSIX do Unix
#include <sys/wait.h> // para waitpid()


void process_command(std::string command) {

    // Comando builtin: tratado direto pela shell, sem criar processo novo
    if (command == "exit")
        exit(0); // encerra a propria shell com codigo de sucesso


    std::string absolute_path = "/bin/" + command; // monta o caminho completo, ex: "/bin/ls"

    // access() verifica coisas sobre um arquivo SEM abri-lo
    // F_OK = "File OK" -> pergunta apenas "esse arquivo existe?"
    // X_OK = "EXECUTE OK" -> pergunta apenas "esse arquivo pode executar?"
    // .c_str() converte a std::string para const char*, que e o que access() espera (funcao em C)
    // access() retorna 0 quando a checagem passa
    if (access(absolute_path.c_str(), F_OK) == 0) { // Se arquivo existe no diretorio

        // X_OK = "eXecute OK" -> pergunta "esse arquivo tem permissao de execucao?"
        if (access(absolute_path.c_str(), X_OK) == 0) { // Arquivo e executavel

           

            pid_t pid = fork();

            if (pid < 0){ 
                std::cout << "Erro de execução!" << std::endl;
                return; 
            } else if (pid == 0){ 

                // Monta o array de argumentos que sera passado ao novo programa.
                // Por convencao Unix, argv[0] e sempre o proprio nome do comando.
                // argv[1] = nullptr marca o FIM da lista (obrigatorio para as funcoes exec*)
                // (char*) e um cast estilo C, forcando a conversao de const char* para char*
                char * argv[2] = {(char *)command.c_str(), nullptr};

                // execve() SUBSTITUI o programa em execucao no processo atual.
                // O processo filho para de rodar o codigo da shell e passa a rodar
                // o programa em absolute_path (ex: /bin/ls), mantendo o mesmo PID.
                // Parametros: (1) caminho do programa, (2) argv montado acima,
                // (3) NULL = nao repassa nenhuma variavel de ambiente ao novo programa.
                // Se execve() der certo, esta linha NUNCA retorna (o processo "virou" outro programa).
                execve(absolute_path.c_str(), argv, NULL);

            } else { // este bloco  roda no processo PAI

                /* Deve adicionar processo filho na lista (std::vector)
                   de processos em execução para gerenciar background. */

                // waitpid bloqueia o pai ate o filho (identificado por pid) terminar.
                // nullptr = nao queremos guardar o status de saida do filho.
                // 0 = flags padrao (comportamento normal de espera bloqueante).
                waitpid(pid, nullptr, 0);
            }

        } else { 
            std::cout << "permission denied: " << command << std::endl;
        }

    } else { 
        std::cout << "Command not found: " << command << std::endl;
    }
}

int main() {

    while (true) {
        std::cout << "$> ";         

        std::string command;        
        getline(std::cin, command);   // le a LINHA INTEIRA digitada (inclui espacos, diferente de cin >>)

        process_command(command);  
    }
    return 0;
}