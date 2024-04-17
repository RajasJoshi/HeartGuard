#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono> // For std::this_thread and std::chrono_literals
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

void readCSV(const std::string& filename, std::vector<double>& x, std::vector<double>& y) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        double x_value, y_value;
        char comma;
        ss >> x_value >> comma >> y_value; // Assuming the CSV file has two values per line separated by a comma
        
        x.push_back(x_value);
        y.push_back(y_value);
    }
    
    file.close();
}

int main() {
    std::vector<double> x_data, y_data;
    std::string filename = "project/max30102/Sample Data/data_9.dat"; // Change to your CSV file's name
    
    // Read initial data
    readCSV(filename, x_data, y_data);

    // Create index vector
    std::vector<double> index(x_data.size());
    for (size_t i = 0; i < index.size(); ++i) {
        index[i] = i;
    }

    // Plot the initial data and set labels
    plt::plot(index, y_data); // Plotting against index
    plt::xlabel("Index");
    plt::ylabel("Red");
    plt::title("MAX30102");
    plt::save("./IR.png"); // Save the plot to basic.png
    plt::clf();

    plt::plot(index, x_data); // Plotting against index
    plt::xlabel("Index");
    plt::ylabel("IR");
    plt::title("MAX30102");
    plt::save("./RED.png"); 

    // Update the plot every second
    while (true) {
        // Clear previous plot
        plt::clf();

        // Read data from CSV file
        x_data.clear();
        y_data.clear();
        readCSV(filename, x_data, y_data);

        // Update each index every second
        for (size_t i = 0; i < y_data.size(); ++i) {
            // Clear previous plot
            plt::clf();

            // Update the y value for the current index
            y_data[i] += 1; // Increment y value by 1 (replace this with your desired update logic)

            // Plot the updated data against index
            plt::plot(index, y_data);
            plt::pause(1); // Pause for 1 second
        }
    }

    return 0;
}
