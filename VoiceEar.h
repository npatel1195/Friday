#pragma once
#include <QObject>
#include <QAudioSource>
#include <QMediaDevices>
#include <QTimer>
#include <QVector>
#include "whisper.h"

class VoiceEar : public QObject {
    Q_OBJECT
public:
    explicit VoiceEar(QObject *parent = nullptr);
    ~VoiceEar();
    void startListening();
    void stopListening();

signals:
    void heardCommand(const QString &text);
    void listeningStateChanged(bool isRecording);

private slots:
    void processAudio();
    void onSilence();

private:
    void transcribe();
    QAudioSource *input = nullptr;
    QIODevice *stream = nullptr;
    QVector<float> buffer;
    QTimer *silenceTimer;
    whisper_context *ctx = nullptr;
    bool isRecording = false;
};
