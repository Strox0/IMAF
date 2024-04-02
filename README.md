---

# IMAF: ImGui & OpenGL Application Framework

IMAF is a C++ framework designed to streamline the development of applications using ImGui and OpenGL. It provides an easy-to-use abstraction for initializing and setting up applications, with additional features such as DPI awareness to enhance user interface scalability and accessibility.

## Features

- **Simplified Initialization**: IMAF abstracts the initialization process for ImGui and OpenGL, making it easier to start developing applications.
- **DPI Awareness**: Supports DPI scaling, ensuring your application's UI remains crisp and accessible on high-resolution displays.
- **Extensible Panel System**: Utilizes a panel-based system for organizing UI components, making it straightforward to structure and manage complex interfaces.
- **ImGui & OpenGL Integration**: Seamlessly integrates ImGui for UI development and OpenGL for graphics rendering, providing a cohesive development experience.

## Getting Started

### Prerequisites

Ensure you have the following dependencies available in your project:
- ImGui
- OpenGL
- GLFW

These dependencies are included in the repository, within the `vendor` directory.

### Installation

1. Clone the IMAF repository or download the source code.
2. Copy the `src` directory and the entire `vendor` directory to your project.
3. Include `Application.h` in your project to start using IMAF.

   ```cpp
   #include "IMAF/Application.h"
   ```

### Usage

The `TestApp` directory contains an example application demonstrating how to use IMAF. Here's a quick overview:

1. **Create Your Application Layer**: Derive a class from `IMAF::Panel` and override the `UiRender` method to define your UI.

   ```cpp
   class ExampleLayer : public IMAF::Panel
   {
   public:
       void UiRender() override
       {
           // Define your ImGui UI here
       }
   };
   ```

2. **Initialize and Run Your Application**: Create an instance of `IMAF::Application`, configure its properties, add your panels, and call `Run`.

   ```cpp
   int main()
   {
       IMAF::AppProperties props;
       // Configure properties as needed

       IMAF::Application app(props);
       std::shared_ptr<ExampleLayer> layer = std::make_shared<ExampleLayer>();
       app.AddPanel(layer);

       app.Run();
       return 0;
   }
   ```

For a comprehensive example, refer to the `TestApp` implementation in the repository.

## License

IMAF is released under the MIT License. This permits a broad spectrum of use cases, from private development to commercial application, provided the license and copyright notice accompany the software.

For more details, see the [LICENSE](LICENSE) file in the project repository.

---