#include "neuralnetwork.h"
#include "tetris.h"

#include <functional>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

int main()
{
    try
    {
        const int userChoice = std::invoke([]() {
            int choice = 0;
            while (choice != 1 && choice != 2 && choice != 3)
            {
                std::cout << "Choose from the following options:\n"
                    << "1: Play Tetris (player controlled)\n"
                    << "2: Play Tetris (AI controlled)\n"
                    << "3: Train AI\n";

                std::string userInput;
                std::getline(std::cin, userInput);
                std::stringstream(userInput) >> choice;
            }

            return choice;
        });

        NeuralNetwork neuralNetwork;
        switch (userChoice)
        {
        case 1:
            Tetris::play();
            break;

        case 2:
            Tetris::play();
            break;

        case 3:
            neuralNetwork.loadFromFile("neural_network.txt");
            neuralNetwork = train(neuralNetwork, "training_data.bin");
            neuralNetwork.saveToFile("neural_network.txt");
            break;

        default:
            throw std::domain_error{ "Invalid user menu choice" };
        }

        return 0;
    }
    catch (const std::exception& error)
    {
        std::cout << error.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        std::cout << "Unhandled exception" << std::endl;
        return -2;
    }
}
