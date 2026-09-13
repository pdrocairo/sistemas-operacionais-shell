# Shell — Sistemas Operacionais

Implementação de um interpretador de linha de comando (shell) em C++, desenvolvida como atividade prática da disciplina de Sistemas Operacionais (IFRN Campus Natal-Central), sob orientação do professor Jorgiano Marcio Bruno Vidal.

## Sobre o projeto

A shell funciona como um laço de leitura e execução de comandos (`REPL`): lê uma linha digitada pelo usuário, separa comando e argumentos, e executa esse comando — seja ele um **comando interno** (tratado pela própria shell) ou um **comando externo** (um programa executável procurado no sistema).

## Funcionalidades

### Parte 1 — Básico

- Laço infinito com prompt `$` (configurável via variável de ambiente `PS1`, se definida)
- Leitura de uma linha por vez da entrada padrão, separando comando e argumentos (espaço como delimitador)
- Execução de comandos externos via `fork()` + `execve()`, com o processo pai aguardando o término do filho (`waitpid()`)
- Leitura de variáveis de ambiente: `PATH` (usada na busca de comandos externos) e `HOME` (usada pelo `cd` sem argumento)

### Parte 2 — Comandos internos

| Comando | Descrição |
|---|---|
| `exit [n]` | Encerra a shell com o código de saída `n` (padrão: `0`) |
| `pwd` | Mostra o diretório atual |
| `cd [dir]` | Muda para o diretório `dir`; sem argumento, vai para `$HOME` |
| `history` | Mostra os últimos 10 comandos digitados, numerados de `0` (mais recente) a `9` (mais antigo) |
| `history -c` | Apaga todo o histórico |
| `history [offset]` | Reexecuta o comando salvo no índice `offset` |

### Parte 3 — Busca em PATH

Comandos externos não são restritos a um único diretório fixo: a shell percorre todos os diretórios listados na variável de ambiente `PATH` (separados por `:`), na ordem em que aparecem, e usa o primeiro executável encontrado com esse nome. Caso o comando digitado já contenha uma barra (`/`) — por exemplo `./programa` ou `/usr/bin/ls` — a shell testa esse caminho diretamente, sem consultar o `PATH`.

## Como compilar

```bash
g++ -std=c++17 -Wall shell.cpp -o shell
```

## Como executar

```bash
./shell
```

## Exemplo de uso

```
$pwd
/home/usuario
$cd /tmp
$pwd
/tmp
$ls -la
...
$history
2 pwd
1 cd /tmp
0 pwd
$history 1
cd /tmp
$exit 0
```

## Decisões de projeto

- **Comandos internos têm prioridade sobre externos**: antes de procurar em `PATH`, a shell verifica se o comando digitado é um dos builtins conhecidos. Isso é necessário, por exemplo, para o `cd` — que precisa alterar o diretório do **próprio processo da shell** (via `chdir()`), e não funcionaria se fosse executado num processo filho criado por `fork()` (a mudança se perderia quando o filho terminasse).
- **Histórico armazenado com `std::deque`**: permite inserção no início e remoção no final em tempo O(1), ideal para manter sempre os últimos 10 comandos.
- **Comando é adicionado ao histórico *depois* de ser processado**: isso evita que o próprio comando `history` apareça listado dentro da sua própria saída.
- **Variáveis de ambiente do processo são repassadas aos comandos externos** via `environ` no `execve()`, garantindo que programas executados (como `ls`, `gcc`, etc.) tenham acesso normal a `PATH`, `HOME` e demais variáveis.

## Limitações conhecidas

- Não há tratamento de caracteres especiais (aspas, TAB, backspace), conforme permitido pelo enunciado — apenas espaço é considerado separador de argumentos.
- Não há suporte a execução em background (`&`), redirecionamento de entrada/saída ou pipes.
- O prompt padrão é fixo em `$`; a customização via `PS1` é uma funcionalidade extra opcional.

## Autor

[Pedro Cairo](https://github.com/pdrocairo)