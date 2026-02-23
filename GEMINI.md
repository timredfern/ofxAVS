# Gemini's Project Instructions for ofxAVS

This document outlines the operating instructions and conventions for my work on the `ofxAVS` project. It is adapted from the original `CLAUDE.md` and incorporates project-specific rules with my core capabilities.

## 1. Core Objective

My primary goal is to assist in porting the Advanced Visualization Studio (AVS) to a modern C++ and openFrameworks addon (`ofxAVS`). This involves understanding the original `vis_avs` codebase and faithfully adapting its features and behaviors into the `ofxAVS` architecture.

## 2. Development & Verification Workflow

1.  **Understand & Strategize:** I will analyze the request and the existing codebase, particularly the original AVS implementation in `vis_avs/`, before implementing any features.
2.  **Plan & Seek Approval:** After gathering necessary information, I will formulate a plan and *always* seek explicit user approval before translating that plan into code changes or modifications. Do not switch from an information-gathering phase to an implementation phase without this approval.
3.  **Implement:** I will write code that adheres to the strict project conventions outlined below.
4.  **Build & Test:** After making changes, I will always attempt to build the relevant example (`chain` or `simple`) to ensure the code compiles.
5.  **Verify:** I will confirm that the changes work as intended by running the application. **A task is not "done" or "fixed" until it has been successfully built and its functionality verified through execution.**

## 3. Critical Project Rules & Conventions

### Fidelity to the Original AVS

*   **Absolute Priority:** I will ALWAYS stay true to the original AVS implementation. My goal is a faithful port, not an enhancement.
*   **No New Features:** I will not add my own improvements, enhancements, or features that did not exist in the original AVS.
*   **Research First:** Before implementing any feature, I will research its original behavior by analyzing the Windows AVS code in `../vis_avs/avs/vis_avs/`.
*   **UI and Behavior Matching:** Dialog layouts, control types (radio buttons, checkboxes), and all visual/functional behaviors will be made to match the original AVS implementation.

### Build Environment

*   **NEVER Hardcode Paths:** I will always use the `$OF_ROOT` environment variable for the openFrameworks path in build files like `config.make`. I will not commit hardcoded, system-specific paths.

**Example Build Process:**
```bash
export OF_ROOT=/path/to/your/openFrameworks
cd examples/chain
make
```

### UI Parameter Naming

*   **CRITICAL:** To prevent "unsupported control type" errors, parameter names passed to `setup_parameters()` MUST **exactly match** the corresponding UI control IDs from the layout definition.
*   **Radio Button Groups:** Each radio button in a group will be treated as a separate boolean parameter, with the name matching its control ID.

### Copyright Headers

All new `.cpp` and `.h` source files MUST include the correct copyright header at the top of the file.

*   **For AVS-derived files (in `libs/avs_lib/`):**
    ```cpp
    // avs_lib - Portable Advanced Visualization Studio library
    // Based on Advanced Visualization Studio by Nullsoft, Inc.
    // Original AVS Copyright (C) 2005 Nullsoft, Inc.
    // C++20 port Copyright (C) 2025 Tim Redfern
    // Licensed under MIT License - see LICENSE file in repository root
    ```

*   **For `ofxAVS` and original files:**
    ```cpp
    // ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
    // Copyright (C) 2025 Tim Redfern
    // Licensed under MIT License - see LICENSE file in repository root
    ```

## 4. Communication Style

*   **Fact-Based:** I will base my responses on the facts available to me in the codebase and through my tools.
*   **Clarity and Conciseness:** I will be direct and focus on the technical aspects of the task.
*   **Problem-Oriented:** In line with the project's preference, I will highlight potential issues, risks, and areas that need further work.
