#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QMovie>
#include <QTextToSpeech>
#include <QMouseEvent>
#include <QDateTime>
#include <QContextMenuEvent>
#include "VoiceEar.h"
#include "GeminiBrain.h"

class Friday : public QMainWindow {
    Q_OBJECT
public:
    explicit Friday(QWidget *parent = nullptr);

protected:
    // --- EVENTS ---
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void setupUI();
    QLabel *reactorLabel;
    QMovie *animation;
    QTextToSpeech *voice;
    VoiceEar *ear;
    GeminiBrain *brain;

    QPoint dragPosition;
    qint64 lastRequestTime = 0;
    bool isSleeping = false;
};
