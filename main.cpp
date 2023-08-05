#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <wordexp.h>
#include <cerrno>
#include <regex>

bool isSafeInput(const std::string& input) {
    // Allow empty input or use a regular expression to check for alphanumeric characters and spaces only
    return input.empty() || std::regex_match(input, std::regex("^[a-zA-Z0-9 ]{1,100}$"));
}

void runShell(bool allowExternalCommands) {
    using_history();

    std::string input;
    char* raw_input;
    while ((raw_input = readline("PORTAL> ")) != nullptr) {
        input = raw_input;
        free(raw_input);

        if (input == "exit") {
            break;
        }

        add_history(input.c_str());

        if (!isSafeInput(input)) {
            std::cerr << "Unsafe input detected!" << std::endl;
            continue;
        }

        std::vector<std::string> args;
        char* token = std::strtok(const_cast<char*>(input.c_str()), " ");
        while (token != nullptr) {
            args.emplace_back(token);
            token = std::strtok(nullptr, " ");
        }

        if (!args.empty()) {
            if (args[0] == "pwd") {
                char cwd[4096];
                if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                    std::cout << cwd << std::endl;
                } else {
                    perror("pwd");
                }
                continue;
            }

            if (args[0] == "echo") {
                for (size_t i = 1; i < args.size(); ++i) {
                    std::cout << args[i] << " ";
                }
                std::cout << std::endl;
                continue;
            }

            if (args[0] == "cd") {
                if (args.size() > 1) {
                    if (args[1] == "~") {
                        wordexp_t exp_result;
                        if (wordexp("~", &exp_result, 0) == 0) {
                            chdir(exp_result.we_wordv[0]);
                            wordfree(&exp_result);
                        } else {
                            perror("wordexp");
                        }
                    } else {
                        if (chdir(args[1].c_str()) != 0) {
                            perror("cd");
                        }
                    }
                } else {
                    std::cerr << "Usage: cd <directory>" << std::endl;
                }
            } else if (args[0] == "mycommand") {
                std::cout << "It works!" << std::endl;
            } else if (allowExternalCommands) { // Check the boolean flag
                pid_t child_pid = fork();

                if (child_pid == 0) {
                    rl_cleanup_after_signal();
                    std::vector<char*> c_args;
                    for (const auto& arg : args) {
                        c_args.push_back(const_cast<char*>(arg.c_str()));
                    }
                    c_args.push_back(nullptr);

                    // execvp() may not return if successful, so only handle errors
                    if (execvp(c_args[0], c_args.data()) == -1) {
                        // Print a more detailed error message using perror
                        perror("execvp");
                        // Exit with a failure status
                        exit(EXIT_FAILURE);
                    }
                } else if (child_pid < 0) {
                    // Print fork error and continue the loop
                    std::cerr << "Fork failed: " << strerror(errno) << std::endl;
                } else {
                    int status;
                    if (waitpid(child_pid, &status, 0) == -1) {
                        // Print waitpid error and continue the loop
                        perror("waitpid");
                    }
                }
            } else {
                std::cerr << "Command not found." << std::endl;
            }
        }
    }
}

int main() {
    bool allowExternalCommands = false; // Set this to true to allow external command execution
    runShell(allowExternalCommands);
    return 0;
}
