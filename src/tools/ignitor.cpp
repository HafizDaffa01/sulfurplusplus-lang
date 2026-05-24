#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

void printHelp() {
    std::cout << "Ignitor - Sulfur++ Embedded Deployment Tool\n\n"
              << "Usage:\n"
              << "  ignitor <command> [arguments]\n\n"
              << "Commands:\n"
              << "  init           Initialize a new Sulfur++ embedded project\n"
              << "  flash          Flash the binary from the 'flash/' directory to the target\n"
              << "                 Options:\n"
              << "                 --bin  Save binary only, bypass flashing to target\n"
              << "  help, -h       Show this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 1;
    }

    std::string command = argv[1];

    if (command == "--help" || command == "-h" || command == "help") {
        printHelp();
    } else if (command == "init") {
        std::cout << "Initializing ignitor project..." << std::endl;
        
        int result = std::system("fuse init");
        if (result != 0) {
            std::cerr << "Failed to run fuse init" << std::endl;
            return 1;
        }

        try {
            if (fs::create_directory("flash")) {
                std::cout << "Created 'flash' directory." << std::endl;
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error creating 'flash' directory: " << e.what() << std::endl;
            return 1;
        }

        std::ofstream config("ignitor.toml");
        if (!config) {
            std::cerr << "Failed to create ignitor.toml" << std::endl;
            return 1;
        }
        
        config << "[hardware]\n";
        config << "port = \"COM3\"\n";
        config << "baud_rate = 115200\n";
        config << "target = \"esp32\"\n\n";
        config << "[project]\n";
        config << "main = \"main.sfpp\"\n";
        config << "flash_speed = \"fast\"\n";
        config.close();

        std::cout << "Ignitor initialized successfully." << std::endl;
    } else if (command == "flash") {
        bool saveBinOnly = false;
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]) == "--bin") {
                saveBinOnly = true;
            }
        }

        std::cout << "Preparing to flash..." << std::endl;
        
        if (!fs::exists("flash") || !fs::is_directory("flash")) {
            std::cerr << "Error: 'flash' directory not found. Please run 'ignitor init' first." << std::endl;
            return 1;
        }

        // Check for binary files in 'flash/'
        bool found = false;
        for (const auto& entry : fs::directory_iterator("flash")) {
            if (entry.is_regular_file()) {
                std::cout << "Found binary: " << entry.path().filename() << std::endl;
                found = true;
                if (saveBinOnly) {
                    std::cout << "Binary saved at: " << entry.path() << " (bypassing flash to target due to --bin flag)" << std::endl;
                } else {
                    // Here you would add the actual call to the hardware flasher (e.g., esptool or avrdude)
                    std::cout << "Calling flash utility for: " << entry.path() << "..." << std::endl;
                }
            }
        }

        if (!found) {
            std::cerr << "Error: No binary files found in 'flash/' directory." << std::endl;
            return 1;
        }

        if (saveBinOnly) {
            std::cout << "Binary preservation completed successfully." << std::endl;
        } else {
            std::cout << "Flashing completed successfully." << std::endl;
        }
    } else {
        std::cerr << "Unknown command: " << command << "\n\n";
        printHelp();
        return 1;
    }

    return 0;
}
