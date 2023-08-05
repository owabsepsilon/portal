#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h> // Include readline header
#include <readline/history.h> // Include history header
#include <wordexp.h>

void runShell() {
    using_history();

    std::string input;
    char* raw_input;
    while ((raw_input = readline("PORTAL> ")) != nullptr) {
        input = raw_input;
        free(raw_input);

        if (input == "exit") {
            break; // Exit the shell
        }

        add_history(input.c_str());

        std::vector<std::string> args;
        char* token = std::strtok(const_cast<char*>(input.c_str()), " ");
        while (token != nullptr) {
            args.emplace_back(token);
            token = std::strtok(nullptr, " ");
        }

                if (args[0] == "pwd") {
                    char cwd[4096];
                    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                        std::cout << cwd << std::endl;
                    } else {
                        perror("pwd");
                    }
                    continue; // Continue to next iteration of the loop
                }
                
                if (args[0] == "echo") {
                    for (size_t i = 1; i < args.size(); ++i) {
                        std::cout << args[i] << " ";
                    }
                    std::cout << std::endl;
                    continue; // Continue to next iteration of the loop
                }
                
                
        if (!args.empty()) {
                if (args[0] == "cd") {
                if (args.size() > 1) {
                    if (args[1] == "~") {
                        wordexp_t exp_result;
                        wordexp("~", &exp_result, 0);
                        chdir(exp_result.we_wordv[0]);
                        wordfree(&exp_result);
                    } else {
                        if (chdir(args[1].c_str()) != 0) {
                            perror("cd"); // Print error message
                        }
                    }
                } else {
                    std::cerr << "Usage: cd <directory>" << std::endl;
                }
            } else if (args[0] == "mycommand") {
                std::cout << "It works!" << std::endl;
            } else {
                pid_t child_pid = fork();

                if (child_pid == 0) {
                    rl_cleanup_after_signal();
                    std::vector<char*> c_args;
                    for (const auto& arg : args) {
                        c_args.push_back(const_cast<char*>(arg.c_str()));
                    }
                    c_args.push_back(nullptr);
                    execvp(c_args[0], c_args.data());

                    perror(c_args[0]);
                    exit(EXIT_FAILURE);
                } else if (child_pid < 0) {
                    perror("fork");
                } else {
                    int status;
                    waitpid(child_pid, &status, 0);
                }
            }
        }
    }
}

int main() {
    runShell();
    return 0;
}
