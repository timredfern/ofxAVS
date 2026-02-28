// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ofMain.h"
#include "ofApp.h"
#include "ofAppGLFWWindow.h"

int main() {
    // Chain/UI window (main application window)
    ofGLFWWindowSettings chain_settings;
    chain_settings.setSize(420, 600);
    chain_settings.setPosition(glm::vec2(100, 100));
    chain_settings.resizable = false;
    chain_settings.title = "AVS Chain";
    auto chain_window = ofCreateWindow(chain_settings);

    // Output/visualization window
    ofGLFWWindowSettings output_settings;
    output_settings.setSize(600, 600);
    output_settings.setPosition(glm::vec2(520, 100));
    output_settings.resizable = true;
    output_settings.title = "AVS Output";
    // No context sharing - texture created in output window context
    auto output_window = ofCreateWindow(output_settings);

    // Create app and store window handles
    auto app = std::make_shared<ofApp>();
    app->setWindows(chain_window, output_window);

    // Bind output window events to app methods
    ofAddListener(output_window->events().draw, app.get(), &ofApp::drawOutput);
    ofAddListener(output_window->events().keyPressed, app.get(), &ofApp::keyPressedOutput);

    ofSetVerticalSync(false); //vital when drawing multiple windows

    // Run main app on chain window
    ofRunApp(chain_window, app);
    ofRunMainLoop();
}
