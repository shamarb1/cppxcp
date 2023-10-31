# What is XCP project

[XCP](https://www.asam.net/standards/detail/mcd-1-xcp/) is an acronym for (Universal(X) Measurement and Calibration Protocol), that is essentially an interface standardized by ASAM to read(measure) and write(calibrate) from the memory of ECU. The "X" in XCP characterizes a measurement and calibration protocol that is transport layer agnostic.

In this XCP project, we use Ethernet as the transport layer and hence the implementation case is "XCP on Ethernet". Although traditionally in XCP documentations, the end-node that is being calibrated is often illustrated an ECU, XCP can quite as well be implemented for measuring from and calibrating any software application, a HPA application for example.

The **xcp-slave** module implements the slave component of the XCP protocol.
It compiled as a shared dynamically linked library which must be integrated into every app for which XCP services shall be provided.
Xcp can be used both during development (measurement and calibration) and as a data collection system for production vehicles
(measurement only). Examples of Xcp masters typically used during development is ATI Vision, ETAS INCA & Vector CANápe.
This slave supports **two** masters simultaneously.
More about XCP and XCP Slave you can read [here](https://pt-gerrit.volvocars.biz/plugins/gitiles/diag/xcp/+/refs/heads/master/doc/).

# For Clients
## How to integrate XCP solution
1. Reserve an available port number for your application in a range [30000..30489] [here](https://confluence.volvocars.biz/display/ARTVDDP/Apps+using+XCP-slave).
2. Add a dependency to cs-module.yml:
    ```
    dependencies:
      xcp-slave:
        min-version: 0.0.1
    ```
3. After you do `cs install`, you'll find `${XCP_SLAVE_BASE_DIR}/cmake/xcp_integration.cmake` module. Include it like this:
    ```
    list(APPEND CMAKE_MODULE_PATH ${XCP_SLAVE_BASE_DIR}/cmake)
    include(xcp_integration)
    ```
    or
    ```
    include(./cmake/xcp_integration.cmake)
    ```
4. Use a special function `target_link_library_xcp` to link the library, initialize, and turn on/off other capabilities.
    ```
    target_link_library_xcp(
        ${YOUR_TARGET_NAME}  # target to which the library is linked to;
        UDP_PORT 30000       # unique port number in a range [30000..30489];
        CALIBRATION          # [optional] flag turns ON calibration capabilities. OFF if absent;
        A2L_GENERATION       # [optional] flag turns ON A2L and S19 file generation. OFF if absent;
        BASE_DIR ""          # [optional] path to the root directory. Uses ${XCP_SLAVE_BASE_DIR} if absent or empty;
        INCLUDE_DIR ""       # [optional] path to header files. Uses ${XCP_SLAVE_INCLUDE_DIR} if absent or empty;
        LIB ""               # [optional] path to *.so file. Uses ${XCP_SLAVE_LIBXCP_SLAVE} if absent or empty;
    )
    ```
    After the call, you will see a status message during CMake configuration steps. Check this status message to verify your intentions are achieved.

    **Example 1.** This issues in terminal:
    ```
    target_link_library_xcp(${YOUR_TARGET_NAME} UDP_PORT 30000)
    ...
    XCP library linked successfully on UDP port 30000 without calibration and without A2L generation.
    ```
    **Example 2.** The next the call issues:
    ```
    target_link_library_xcp(${YOUR_TARGET_NAME} UDP_PORT 30489 CALIBRATION A2L_GENERATION)
    ...
    XCP library linked successfully on UDP port 30489 with calibration and with A2L generation.
    ```
    **Example 3.** In order to change the path to header files or shared library you can choose one of the ways:
    ```
    set(XCP_SLAVE_BASE_DIR ../xcp_slave)
    set(XCP_SLAVE_INCLUDE_DIR ../xcp_slave/include)
    set(XCP_SLAVE_LIBXCP_SLAVE ../xcp_slave/lib/libxcp_slave.so)

    target_link_library_xcp(${YOUR_TARGET_NAME} UDP_PORT 30000)
    ```
    or
    ```
    target_link_library_xcp(
        ${YOUR_TARGET_NAME}
        UDP_PORT 30000
        BASE_DIR "../xcp_slave"
        INCLUDE_DIR "../xcp_slave/include"
        LIB "../xcp_slave/lib/libxcp_slave.so"
    )
    ```
5. Include `xcp.h` in your sources and initialize the library.
    ```
    #include <xcp.h>
    int main() {
        init_data_t xcp_init_data = XCP_CREATE_INIT_DATA(); // Define an initialization structure
        xcp_init_data.log_level = 2;                        // [optional] Make some custom initializations
        XcpInit(&xcp_init_data);                            // Initialize the library
        return 0;
    }
    ```
See **xcp_example_app** project in the root as an example.

You are free to ask any questions regarding XCP the team [The Interfacers](https://confluence.volvocars.biz/display/ARTVDDP/The+Interfacers) at all stages of your project, e.g. design (we will help you to identify measurement and calibration capabilities of your project), implementation (help you to integrate XCP solution) and release (since XCP works with "information", there are some legal aspects of the data processing from production vehicles).
# For Maintainers
If you are new here, just run **build_local.sh** script from the root right after the cloning the repository. It will install missing tools (**clang-tidy**, **clang-format**, **cmake-format**) to the system (except **Polyspace tools**, you have to install them using *installation steps* from the [manual](https://confluence.volvocars.biz/display/ARTPLATSWS/Polyspace+information+and+coding+guidelines)), build the project locally, run code formatting and analysis, and run tests.
## Repository Structure
```
├── cmake                            - common CMake modules
│   ├── clang_format.cmake             - run clang-format tool for particular target
│   ├── clang_tidy.cmake               - run clang-tidy tool for particular target
│   ├── cmake_format.cmake             - run clang-format for the whole repository
│   ├── colorized_messages.cmake       - extend built-in message function with a color parameter
│   ├── enabled_projects.cmake         - add new sub-projects
│   ├── polyspace_tools.cmake          - run Polyspace tools for the whole repository
│   └── unittests.cmake                - run unit tests with corresponding `lcov` report
├── projects                         - sub-projects
│   ├── common                         - common header and source files for all sub-projects
│   |   ├── include                      - header files
│   |   └── src                          - source files
│   └── udp                            - UDP static library, includes socket and server implementation
│   |   ├── include                      - header files for export
│   |   ├── lib                          - source files
│   |   └── tests                        - unit tests
│   └── xcp                            - XCP static library
│       ├── include                      - header files for export
│       ├── lib                          - source files
|       |   ├── protocol_layer             - protocol layer implementation
|       |   └── transport_layer            - transport layer implementations
|       |       └── ethernet                 - ethernet (UDP/IP) layer
│       └── tests                        - unit tests
├── tools                            - common scripts, files, materials, etc
│   └── install_vscode_extensions.sh   - install VS Code extensions needed for the project
├── xcp_example_app                  - old C implementation of example application with integrated XCP slave
│   ├── cmake                          - CMake modules related to example application
│   |   └── xcp_integration.cmake        - integrate XCP library into the example application
│   ├── src                            - source files
│   |   └── *.hpp;*.cpp
│   └── CMakeLists.txt               - CMake configuration related to example application
├── xcp_slave                        - old C implementation of XCP slave library
│   ├── include                        - header files for export
│   |   └── *.h
│   ├── lib                            - source files
│   |   └── *.h;*.c
│   ├── tools                          - scripts, files, materials, etc related to the library
│   |   ├── a2l_templates                - template files used by VccA2lMerge during the A2L file generation
│   |   |   ├─ calibration                 - template files used when calibration is ON
│   |   |   |  ├─ hpa.a2l                    - template file used for QNX
│   |   |   |  └─ vhpa.a2l                   - template file used for Linux (emulator)
│   |   |   ├─ hpa.a2l                     - template file used for QNX without calibration
│   |   |   └─ vhpa.a2l                    - template file used for Linux (emulator) without calibration
│   |   ├── linker_scripts               - scripts that run on linking step of the client application
│   |   |   └─ sections.ld                 - assign addresses of the sections of the client application binary
│   |   └── vcc_a2l_tools                - tools that run through phases of making A2L and S19 files
│   └── CMakeLists.txt                 - CMake configuration related to the library
├── .clang-format                    - configuration file for clang-format tool
├── .clang-tidy                      - configuration file for clang-tidy tool
├── .cmake-format                    - configuration file for cmake-format tool
├── .gitignore                       - file with excludes for git tool
├── build_local.sh                   - perform local build, format, analysis, test. Check and install needed tools to the system
├── CMakeLists.txt                   - common CMake configuration
└── readme.md                        - this file
```
`xcp_example_app` and `xcp_slave` projects are going to be removed once the new C++ implementation is ready.
## CMake
### Design
CMake configuration is designed to build everything in a few familiar commands. Next example only builds XCP slave library.
```
mkdir build && cd build
cmake ..
make
```
### XCP project-related variables
To better manage your build, there are special XCP project-related variables.
#### **LOCAL_BUILD** : BOOL
Values: `ON`, `OFF`. `OFF` by default.

Triggers local build for the Linux machine for debug purposes.

`ON` ─ performs debug build of all enabled projects on Linux.
Makes available PROJECTS, TOOLS, and TESTS variables from below for usage.

`OFF` ─ performs release build of **xcp_slave** only. Equivalent to not setting this variable.
Forces next variables to be set:
  - PROJECTS = xcp_slave
  - TOOLS = ""
  - TESTS = ""

Example:
```
cmake -DLOCAL_BUILD=ON ..
```
#### **PROJECTS** : STRING LIST
Values: `all`, `xcp_slave`, `xcp_example_app`, `udp`. `""` (empty) by default. It only works when **LOCAL_BUILD** variable is ON.

Presents the list of the enabled projects scheduled to build separated by semicolon.

`""`(empty) ─ equivalent to not setting this variable. Default value.

`all` ─ includes all the types of tests. Equivalent to `xcp_slave;xcp_example_app;udp_socket`.

`xcp_slave` ─ includes **xcp_slave** project to build.

`xcp_example_app` ─ includes **xcp_example_app** project to build. Has dependency `xcp_slave`.

`udp` ─ includes **udp** project to build.

Note: If some project has any dependency projects, they also will be build first.

Example:
```
cmake -DLOCAL_BUILD=ON -DPROJECTS=all ..
cmake -DLOCAL_BUILD=ON -DPROJECTS=xcp_example_app ..
```
#### **TOOLS** : STRING LIST
Values: `all`, `clang-format`, `cmake-format`, `clang-tidy`, `bug-finder`, `code-prover`. `""` (empty) by default.
It only works when **LOCAL_BUILD** variable is ON.

Presents the list of development tools separated by semicolon. Enables run of certain tools against source files.

`""`(empty) ─ equivalent to not setting this variable. Default value.

`all` ─ includes all the tools. Equivalent to `clang-format;cmake-format;clang-tidy;bug-finder;code-prover`.

`clang-format` ─ applies clang formatting to source files of the targets which have included `clang_format` function to its CMake configuration.
Uses **.clang-format** configuration file in the root directory.

`cmake-format` ─ applies cmake formatting to each CMake file in the repository.
Uses **.cmake-format** configuration file in the root directory.

`clang-tidy` ─ runs clang static code analysis on source files of the targets which have included `clang_tidy` function to its CMake configuration.
Uses **.clang-tidy** configuration file in the root directory.
Generate reports for every target and store them in **./reports/clang-tidy** folder in format **{target_name}_{timestamp}.log**.

`bug-finder` ─ runs static code analysis using Polyspace Bug Finder tool.
Generate reports and store them in **./reports/polyspace** folder in format **bug_finder_{timestamp}.html**.
> **Note**: This tool only runs in Volvo intranet or under Big-IP VPN. Otherwise you will get an error message
*"Polyspace Bug Finder Prover analysis failed. Check you are in Volvo intranet or VPN is connected."*.

`code-prover` ─ runs static code analysis using Polyspace Code Prover tool.
Generate reports and store them in **./reports/polyspace** folder in format **code_prover_{timestamp}.html**.

> **Note**: This tool only runs in Volvo intranet or under Big-IP VPN. Otherwise you will get an error message
*"Polyspace Code Prover analysis failed. Check you are in Volvo intranet or VPN is connected."*.

> **Warning**: Code Prover has known bug with including headers and it is recommended to analyze each project separately.
Otherwise, it will halt with a misleading error message from above.

`lcov` ─ runs coverage analysis for enabled projects. Only works in pair with unit tests (`unit` shall be in the **TESTS** variable list).
Generate reports and store them in **./reports/lcov/{target_name}_{timestamp}**. Then, you can open `index.html` from inside in the browser.

Examples:
```
cmake -DLOCAL_BUILD=ON -DTOOLS="" ..
cmake -DLOCAL_BUILD=ON -DTOOLS=all ..
cmake -DLOCAL_BUILD=ON -DTOOLS=clang-format\;cmake-format ..
cmake -DLOCAL_BUILD=ON -DTOOLS="bug-finder;clang-tidy" ..
```
#### **TESTS** : STRING LIST
Values: `all`, `unit`. `""` (empty) by default. It only works when **LOCAL_BUILD** variable is ON.

Presents the list of test types separated by semicolon.

`""`(empty) ─ equivalent to not setting this variable. Default value.

`all` ─ includes all the types of tests. Equivalent to `unit`.

`unit` ─ run unit tests.

Example:
```
cmake -DLOCAL_BUILD=ON -DTESTS=all ..
cmake -DLOCAL_BUILD=ON -DTESTS=unit ..
```
### Modules
#### Add new projects
Presented in `enabled_projects.cmake`.
Function `add_project()` shall be used in the root `CMakeLists.txt` file. Signature:
```
add_project(<project> [<project-dir>])
```
 - **project** is the name of the new project. The name shall match the one which is used in PROJECTS CMake variable list. It may differ from the actual target name.

 - **project-dir** [optional] is the directory of the project. Omitting it, it would suppose the project directory has the same name as the project's name.


 The function obeys **PROJECTS** variable list and adds only those projects which are specified in the list.

 It also sets *global property* **ENABLED_PROJECTS** which holds the final list of enabled projects that will be build.
#### Add unittests
Presented in `unittests.cmake`.
Function `target_unittests()` creates a new target with the name in format **unittests_{target_name}** then run it after the build. Signature:
```
target_unittests(<target> SOURCES <source...> [INCLUDES <include-dir...>] [LINKS <linked-library...>])
```
 - **target** is the name of the target on which the tests are based. It shall not be empty.

 - **SOURCES** is the list of source files related to the test only. The source files will be implicitly added from the target. There shall be at least one source file with `main` function.

 - **INCLUDES** [optional] is the list of directories with included header files. The include directories will be implicitly added from the target.

 - **LINKS** [optional] is the list of linked libraries. The linked libraries will be implicitly added from the target. It is also implicitly added libraries `gtest`, and `gcov` if `lcov` tool is specified in TOOLS variable list.

 The function obeys **TESTS** variable list and runs only those tests which are specified in the list.