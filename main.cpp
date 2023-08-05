#include <iostream>
#include <cstdlib>
#include <cstdio>
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
#include <dirent.h>
#include <sys/stat.h> // Include this header for file status information
#include <iomanip> // Include this header for setw()
#include <termcap.h>
#include <fstream>

bool hasANSISupport() {
  const char* term = getenv("TERM");
  if (term == nullptr) {
    return false;
  }

  char tbuffer[2048];
  if (tgetent(tbuffer, term) != 1) {
    return false;
  }

  return true;
}

bool isDirectoryEmpty(const std::string& path) {
  DIR* dir = opendir(path.c_str());
  if (dir) {
    struct dirent* entry;
    int count = 0;
    while ((entry = readdir(dir)) != nullptr) {
      if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..") {
        continue;
      }
      ++count;
    }
    closedir(dir);
    return count == 0;
  } else {
    perror(("opendir: " + path).c_str());
    return false;
  }
}

bool isSafeInput(const std::string& input) {
  // Allow empty input or use a regular expression to check for alphanumeric characters and spaces only
  return input.empty() || std::regex_match(input, std::regex("^[a-zA-Z0-9 \\-_<>()\"'/\\.]*$"));
}

void touchFile(const std::string& path) {
  std::ofstream file(path);
  if (!file) {
    perror(("touch: " + path).c_str());
  }
}


void makeDirectory(const std::string& path) {
  if (mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
    perror(("mkdir: " + path).c_str());
  }
}

void removeFile(const std::string& path) {
  if (remove(path.c_str()) != 0) {
    perror(("rm: " + path).c_str());
  }
}

void removeDirectory(const std::string& path) {
  DIR* dir = opendir(path.c_str());
  if (dir) {
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..") {
        continue;
      }
      std::string subPath = path + "/" + entry->d_name;
      struct stat fileStat;
      if (stat(subPath.c_str(), &fileStat) == 0) {
        if (S_ISDIR(fileStat.st_mode)) {
          removeDirectory(subPath);
        } else {
          removeFile(subPath);
        }
      }
    }
    closedir(dir);

    if (rmdir(path.c_str()) != 0) {
      perror(("rmdir: " + path).c_str());
    }
  } else {
    perror(("opendir: " + path).c_str());
  }
}

void copyFile(const std::string& source, const std::string& destination) {
  std::ifstream srcFile(source, std::ios::binary);
  std::ofstream dstFile(destination, std::ios::binary);

  if (!srcFile) {
    perror(("cp: " + source).c_str());
    return;
  }

  if (!dstFile) {
    perror(("cp: " + destination).c_str());
    return;
  }

  dstFile << srcFile.rdbuf();
}

void listDirectory(const std::string& path, bool longFormat, bool showHidden) {
  DIR* dir = opendir(path.c_str());
  if (dir) {
    struct dirent* entry;
    std::vector<std::string> entries;

    while ((entry = readdir(dir)) != nullptr) {
      if (!showHidden && entry->d_name[0] == '.') {
        continue; // Skip hidden files if not showing hidden
      }
      entries.push_back(entry->d_name);
    }
    closedir(dir);

    if (!longFormat) {
      size_t maxEntryWidth = 0;
      for (const auto& entryName : entries) {
        maxEntryWidth = std::max(maxEntryWidth, entryName.length());
      }

      const int columns = 80 / (maxEntryWidth + 2); // Adjust the number of columns based on your terminal width

      for (size_t i = 0; i < entries.size(); ++i) {
        const std::string& entryName = entries[i];
        struct stat fileStat;
        if (stat((path + "/" + entryName).c_str(), &fileStat) == 0) {
          if (S_ISDIR(fileStat.st_mode)) {
            // If the entry is a directory, print it in blue
            std::cout << "\033[1;34m" << std::setw(maxEntryWidth) << std::left << entryName << "\033[0m";
          } else {
            std::cout << std::setw(maxEntryWidth) << std::left << entryName;
          }

          if ((i + 1) % columns == 0 || i == entries.size() - 1) {
            std::cout << std::endl;
          } else {
            std::cout << "  ";
          }
        }
      }
    } else {
      for (const auto& entryName : entries) {
        struct stat fileStat;
        if (stat((path + "/" + entryName).c_str(), &fileStat) == 0) {
          if (S_ISDIR(fileStat.st_mode)) {
            // If the entry is a directory, print it in blue
            std::cout << "\033[1;34m" << entryName << "\033[0m";
          } else {
            std::cout << entryName;
          }

          std::cout << " (" << fileStat.st_size << " bytes)" << std::endl;
        }
      }
    }
  } else {
    perror("ls");
  }
}

// current built in commands = ls,pwd,cd,rm,mkdir,touch,cp

void runShell(bool allowExternalCommands) {
  using_history();

  std::string input;
  char* raw_input;
  while ((raw_input = readline("as6> ")) != nullptr) {
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



      if (args[0] == "ls") {
        bool longFormat = false;
        bool showHidden = false;
        for (size_t i = 1; i < args.size(); ++i) {
          if (args[i] == "-l") {
            longFormat = true;
          } else if (args[i] == "-a") {
            showHidden = true;
          }
        }
        if (args.size() > 1 && args[1][0] != '-') {
          listDirectory(args[1], longFormat, showHidden);
        } else {
          listDirectory(".", longFormat, showHidden);
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

      if (args[0] == "cp") {
        if (args.size() < 3) {
          std::cerr << "cp: missing operand\n";
        } else {
          std::string source = args[1];
          std::string destination = args.back();

          copyFile(source, destination);
        }
        continue;
      }

      if (args[0] == "touch") {
        if (args.size() < 2) {
          std::cerr << "touch: missing file operand\n";
        } else {
          for (size_t i = 1; i < args.size(); ++i) {
            touchFile(args[i]);
          }
        }
        continue;
      }

      if (args[0] == "mkdir") {
        if (args.size() < 2) {
          std::cerr << "mkdir: missing operand\n";
        } else {
          for (size_t i = 1; i < args.size(); ++i) {
            if (mkdir(args[i].c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
              perror(("mkdir: " + args[i]).c_str());
            }
          }
        }
        continue;
      }

      if (args[0] == "rm") {
        bool recursive = false;
        bool force = false;
        std::vector<std::string> targets;

        if (args.size() < 2) {
          std::cerr << "rm: missing operand\n";
          return;
        }

        for (size_t i = 1; i < args.size(); ++i) {
          if (args[i] == "-r") {
            recursive = true;
          } else if (args[i] == "-f") {
            force = true;
          } else {
            targets.push_back(args[i]);
          }
        }

        if (targets.empty()) {
          std::cerr << "rm: missing operand\n";
          return;
        }

        for (const std::string& target : targets) {
          struct stat fileStat;
          if (stat(target.c_str(), &fileStat) == 0) {
            if (S_ISDIR(fileStat.st_mode) && recursive) {
              if (isDirectoryEmpty(target)) {
                removeDirectory(target);
              } else {
                std::cerr << "rm: cannot remove '" << target << "': Directory not empty" << std::endl;
              }
            } else if (!S_ISDIR(fileStat.st_mode)) {
              removeFile(target);
            } else {
              std::cerr << "rm: cannot remove '" << target << "': Is a directory (use -r to remove)" << std::endl;
            }
          } else {
            if (!force) {
              perror(("rm: " + target).c_str());
            }
          }
        }
        continue;
      }


      if (args[0] == "ansistatus") {
        if (hasANSISupport()) {
          std::cout << "ANSI ENABLED" << std::endl;
        } else {
          std::cout << "ANSI DISABLED" << std::endl;
        }
        continue;
      }


      if (args[0] == "clear") {
        // Clear the terminal
        std::cout << "\033[2J\033[H";
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
        continue; // Continue the loop after handling cd command
      }

      if (args[0] == "version") {
        std::cout << "APERTURE LABORATORIES SHELL v0.1" << std::endl;
        continue; // Continue the loop after handling version command
      }

      if (allowExternalCommands) { // Check the boolean flag
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
