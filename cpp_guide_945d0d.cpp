// This tutorial teaches how to visualize the Mandelbrot fractal using C++.
// We will learn about:
// 1. Complex number arithmetic (specifically, squaring complex numbers).
// 2. Iterative processes and escape-time algorithms.
// 3. Mapping mathematical results to visual colors.
//
// The Mandelbrot set is a fascinating fractal defined by a simple iterative process.
// For each complex number 'c', we repeatedly apply the function z = z^2 + c,
// starting with z = 0. If the magnitude (distance from the origin) of 'z'
// stays bounded (doesn't grow infinitely large), then 'c' is part of the
// Mandelbrot set. Otherwise, it is not. The colors will represent how quickly
// 'z' escapes to infinity.

#include <iostream> // For outputting information to the console.
#include <complex>  // For using complex numbers. This is a standard C++ library.
#include <vector>   // For storing the pixel data.

// Define the dimensions of our image.
const int IMAGE_WIDTH = 800;
const int IMAGE_HEIGHT = 600;

// Define the region of the complex plane to visualize.
// This is a common viewing window for the Mandelbrot set.
const double MIN_REAL = -2.0;
const double MAX_REAL = 1.0;
const double MIN_IMAGINARY = -1.5;
const double MAX_IMAGINARY = 1.5;

// Maximum number of iterations to perform for each complex number.
// A higher number gives more detail but takes longer to compute.
const int MAX_ITERATIONS = 100;

// Function to calculate the number of iterations before a complex number escapes.
// 'c' is the complex number we are testing.
// Returns the number of iterations, or MAX_ITERATIONS if it doesn't escape.
int mandelbrotIterations(const std::complex<double>& c) {
    // Initialize z to 0. This is the starting point for the iteration.
    std::complex<double> z = 0;

    // The core of the Mandelbrot calculation: z = z^2 + c
    // We perform this iteration up to MAX_ITERATIONS times.
    for (int i = 0; i < MAX_ITERATIONS; ++i) {
        // Calculate z^2. The std::complex library handles this.
        // std::complex<double> z_squared = z * z; // Alternatively, you can do it this way.
        z = std::pow(z, 2) + c; // Use std::pow for squaring, or z * z

        // Check if the magnitude of z has exceeded a certain threshold.
        // If abs(z) > 2, it's guaranteed to escape to infinity.
        // This is a common optimization.
        if (std::abs(z) > 2.0) {
            // If it escapes, return the number of iterations it took.
            // This 'i' value will be used to determine the color.
            return i;
        }
    }
    // If the loop completes without escaping, the point is considered to be
    // within the Mandelbrot set (or very close to it).
    return MAX_ITERATIONS;
}

// Function to map the number of iterations to a color.
// This is a simple grayscale mapping, but can be extended to full color.
// 'iterations' is the result from mandelbrotIterations.
// Returns a simple integer representing intensity (0=black, 255=white).
int mapIterationsToColor(int iterations) {
    if (iterations == MAX_ITERATIONS) {
        // Points inside the Mandelbrot set are typically colored black.
        return 0;
    } else {
        // Points outside the set are colored based on how quickly they escaped.
        // We map the iteration count to a grayscale value.
        // A linear mapping is simple: more iterations means brighter color.
        // The division ensures the value stays within 0-255.
        return static_cast<int>(255 * static_cast<double>(iterations) / MAX_ITERATIONS);
    }
}

int main() {
    // This vector will hold our pixel data. We'll store grayscale intensity.
    // Imagine this as a 2D array of pixels.
    std::vector<std::vector<int>> imageData(IMAGE_HEIGHT, std::vector<int>(IMAGE_WIDTH));

    // Loop through each pixel in our image.
    for (int y = 0; y < IMAGE_HEIGHT; ++y) {
        for (int x = 0; x < IMAGE_WIDTH; ++x) {
            // 1. Map the pixel coordinates (x, y) to a point in the complex plane.
            // We scale and shift the pixel coordinates to fit within our defined
            // MIN/MAX_REAL and MIN/MAX_IMAGINARY ranges.

            // Calculate the real part of the complex number.
            // (x / IMAGE_WIDTH) maps the x-coordinate from [0, IMAGE_WIDTH) to [0.0, 1.0).
            // Then we scale it by (MAX_REAL - MIN_REAL) and shift it by MIN_REAL.
            double real = MIN_REAL + (static_cast<double>(x) / IMAGE_WIDTH) * (MAX_REAL - MIN_REAL);

            // Calculate the imaginary part of the complex number.
            // Similarly, we map the y-coordinate from [0, IMAGE_HEIGHT) to [0.0, 1.0).
            // Note: the y-axis usually increases downwards in images, so we might
            // need to invert it depending on the complex plane orientation. Here,
            // we map y=0 to MAX_IMAGINARY and y=IMAGE_HEIGHT to MIN_IMAGINARY
            // to match the typical view of the Mandelbrot set.
            double imaginary = MAX_IMAGINARY - (static_cast<double>(y) / IMAGE_HEIGHT) * (MAX_IMAGINARY - MIN_IMAGINARY);

            // Create the complex number 'c' for this pixel.
            std::complex<double> c(real, imaginary);

            // 2. Calculate how many iterations it takes for 'c' to escape.
            int iterations = mandelbrotIterations(c);

            // 3. Map the iteration count to a grayscale color.
            int color = mapIterationsToColor(iterations);

            // Store the color in our image data.
            imageData[y][x] = color;
        }
    }

    // --- Example Usage: Outputting the image data ---
    // For a real application, you'd save this data to an image file (like PPM, PNG).
    // Here, we'll just print a small portion or a simplified representation to the console.
    // This demonstrates that we have computed the data.

    std::cout << "Mandelbrot set visualization data generated.\n";
    std::cout << "Image dimensions: " << IMAGE_WIDTH << "x" << IMAGE_HEIGHT << "\n";
    std::cout << "Max iterations: " << MAX_ITERATIONS << "\n";
    std::cout << "\nDisplaying a small preview (first 10x10 pixels):\n";

    for (int y = 0; y < std::min(10, IMAGE_HEIGHT); ++y) {
        for (int x = 0; x < std::min(10, IMAGE_WIDTH); ++x) {
            // Print a character based on the color intensity.
            // '#' for darker areas, '.' for brighter areas.
            if (imageData[y][x] < 50) std::cout << "#"; // Darker, likely in or near the set
            else if (imageData[y][x] < 150) std::cout << "*";
            else std::cout << "."; // Brighter, escaped quickly
        }
        std::cout << "\n";
    }
    std::cout << "\n'#' represents points likely inside the Mandelbrot set.\n";
    std::cout << "'.' represents points that escaped quickly.\n";

    // To actually see the fractal, you would need to write this 'imageData'
    // to a file format that an image viewer can open, like PPM (Portable Pixmap).
    // For example, you could add code here to write to a .ppm file.

    return 0;
}