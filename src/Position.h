//
// Created by erx on 10/25/25.
//

#pragma once
#include <LayerShellQt/Window>
#include <QQuickWindow>
#include <QMargins>
#include <QString>

struct OverlayPosition {
    QString position; // "top-left" | "top-right" | "bottom-left" | "bottom-right" | "custom"
    int marginTop{16};
    int marginRight{16};
    int marginBottom{16};
    int marginLeft{16};
    int x{0}; // custom
    int y{0}; // custom
};

inline void applyOverlayPosition(QQuickWindow *win, const OverlayPosition &cfg) {
    using namespace LayerShellQt;

    auto ls = Window::get(win);

    Window::Anchors anchors = {}; // start with no anchors

    if (cfg.position == "top-left") {
        anchors |= Window::AnchorTop;
        anchors |= Window::AnchorLeft;
        ls->setAnchors(anchors);
        ls->setMargins(QMargins(cfg.marginLeft, cfg.marginTop, 0, 0));
    } else if (cfg.position == "top-right") {
        anchors |= Window::AnchorTop;
        anchors |= Window::AnchorRight;
        ls->setAnchors(anchors);
        ls->setMargins(QMargins(0, cfg.marginTop, cfg.marginRight, 0));
    } else if (cfg.position == "bottom-left") {
        anchors |= Window::AnchorBottom;
        anchors |= Window::AnchorLeft;
        ls->setAnchors(anchors);
        ls->setMargins(QMargins(cfg.marginLeft, 0, 0, cfg.marginBottom));
    } else if (cfg.position == "bottom-right") {
        anchors |= Window::AnchorBottom;
        anchors |= Window::AnchorRight;
        ls->setAnchors(anchors);
        ls->setMargins(QMargins(0, 0, cfg.marginRight, cfg.marginBottom));
    } else {
        // custom (or unknown → treat as custom)
        anchors |= Window::AnchorTop;
        anchors |= Window::AnchorLeft;
        ls->setAnchors(anchors);
        ls->setMargins(QMargins(cfg.x, cfg.y, 0, 0));
    }

    // keep overlay layer
    ls->setLayer(Window::LayerOverlay);
}