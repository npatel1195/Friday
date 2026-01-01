#include "VoiceEar.h"
#include <QCoreApplication>
#include <QtMath>
#include <QDebug>

VoiceEar::VoiceEar(QObject *parent) : QObject(parent) {
    // 1. Load Model
    QString model = QCoreApplication::applicationDirPath() + "/models/ggml-base.en.bin";
    struct whisper_context_params cparams = whisper_context_default_params();
    ctx = whisper_init_from_file_with_params(model.toStdString().c_str(), cparams);

    if(!ctx) qDebug() << "❌ CRITICAL: Whisper model missing at" << model;
    else qDebug() << "✅ Whisper model loaded.";

    // 2. Setup Audio
    QAudioFormat fmt;
    fmt.setSampleRate(16000);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Float);

    QAudioDevice device = QMediaDevices::defaultAudioInput();
    qDebug() << "🎤 Mic:" << device.description();

    input = new QAudioSource(device, fmt, this);

    // 3. Timers
    silenceTimer = new QTimer(this);
    silenceTimer->setSingleShot(true);
    silenceTimer->setInterval(800); // Wait 0.8s for silence
    connect(silenceTimer, &QTimer::timeout, this, &VoiceEar::onSilence);
}

VoiceEar::~VoiceEar() { if(ctx) whisper_free(ctx); }

void VoiceEar::startListening() {
    if(!ctx) return;
    if(input->state() == QAudio::ActiveState) return;

    buffer.clear();
    stream = input->start();
    connect(stream, &QIODevice::readyRead, this, &VoiceEar::processAudio);
    qDebug() << "👂 Ears ON. Waiting for voice...";
}

void VoiceEar::stopListening() {
    if(input) input->stop();
    if(stream) stream->disconnect();
    isRecording = false;
    emit listeningStateChanged(false);
}

void VoiceEar::processAudio() {
    if(!stream) return;
    QByteArray data = stream->readAll();
    const float *pts = reinterpret_cast<const float *>(data.constData());
    int count = data.size() / sizeof(float);

    double sum = 0;
    for(int i=0; i<count; ++i) {
        if(isRecording) buffer.append(pts[i]);
        sum += pts[i] * pts[i];
    }

    float vol = qSqrt(sum/count);

    // DEBUG: If this is 0.0000, your mic is dead.
    // if (vol > 0.0001) qDebug() << "Vol:" << vol;

    // SENSITIVITY: 0.001f is a good balance for Laptop Mics
    if(vol > 0.005f) {
        if(!isRecording) {
            isRecording = true;
            emit listeningStateChanged(true); // Red Ring
            qDebug() << "🗣️ Voice Detected!";
        }
        silenceTimer->start(); // Reset silence timer
    }

    // SAFETY: Force stop if recording gets too long (4 seconds)
    // This prevents it from getting stuck listening to fans/noise
    if(isRecording && buffer.size() > (16000 * 4)) {
        qDebug() << "⚠️ Max time reached, forcing process.";
        onSilence();
    }
}

void VoiceEar::onSilence() {
    if(isRecording) {
        isRecording = false;
        emit listeningStateChanged(false); // Blue Ring
        transcribe();
    }
}

void VoiceEar::transcribe() {
    if(buffer.isEmpty()) return;

    qDebug() << "📝 Transcribing...";

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress = false;
    params.n_threads = 4;

    if(whisper_full(ctx, params, buffer.data(), buffer.size()) == 0) {
        QString text;
        int n = whisper_full_n_segments(ctx);
        for(int i=0; i<n; ++i) text += QString::fromUtf8(whisper_full_get_segment_text(ctx, i));

        text = text.trimmed();

        // Remove hallucinated silence
        text.remove("[silence]", Qt::CaseInsensitive);
        text.remove("(silence)", Qt::CaseInsensitive);
        text = text.trimmed();

        if(!text.isEmpty()) {
            qDebug() << "✅ Heard:" << text;
            emit heardCommand(text);
        } else {
            qDebug() << "❌ Heard only silence.";
        }
    }
    buffer.clear();
}
