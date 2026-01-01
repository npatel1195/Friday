#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QMovie>
#include <QTextToSpeech>
#include <QMouseEvent>
#include <QDateTime>
#include "VoiceEar.h"
#include "GeminiBrain.h"

class Friday : public QMainWindow {
    Q_OBJECT
public:
    explicit Friday(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void setupUI();
    QLabel *reactorLabel;
    QMovie *animation;
    QTextToSpeech *voice;
    VoiceEar *ear;
    GeminiBrain *brain;

    QPoint dragPosition;
    qint64 lastRequestTime = 0;

    // --- NEW STATE VARIABLE ---
    bool isSleeping = false;
};
