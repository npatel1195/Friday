#include "friday.h"
#include "ActionEngine.h"
#include <QTimer>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QDirIterator>
#include <QSettings>
#include <QInputDialog>
#include <QProcessEnvironment>
#include <QMenu>
#include <QContextMenuEvent>

Friday::Friday(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    voice = new QTextToSpeech(this);
    brain = new GeminiBrain(this);
    ear = new VoiceEar(this);

    // ==========================================
    // 🔑 API KEY LOGIC (Startup Check)
    // ==========================================
    QSettings settings("FridayCorp", "FridayAssistant");
    QString finalKey = "";

    // 1. Check Env Var (Priority for Developers)
    QString envKey = qEnvironmentVariable("OPENAI_API_KEY");
    if (!envKey.isEmpty() && envKey.startsWith("sk-")) {
        finalKey = envKey;
        qDebug() << "🔑 Using Environment Variable Key.";
    }
    // 2. Check Saved Settings (Priority for Users)
    else {
        finalKey = settings.value("openai_key").toString();
    }

    // 3. Validation: If key is missing or looks wrong, FORCE PROMPT
    if (finalKey.isEmpty() || !finalKey.startsWith("sk-")) {
        bool ok;
        QString text = QInputDialog::getText(this, "Friday Setup",
                                             "Please enter your OpenAI API Key (sk-...):",
                                             QLineEdit::Normal, "", &ok);
        if (ok && !text.isEmpty()) {
            finalKey = text.trimmed();
            settings.setValue("openai_key", finalKey); // Save it forever
            voice->say("Key saved.");
        }
    }

    // 4. Send to Brain
    if (finalKey.isEmpty()) {
        voice->say("I need an API key to function, sir.");
    } else {
        brain->setApiKey(finalKey);
    }
    // ==========================================

    // 1. AUDIO LOOP CONTROL
    connect(voice, &QTextToSpeech::stateChanged, this, [this](QTextToSpeech::State state){
        if (state == QTextToSpeech::Speaking) {
            ear->stopListening();
            animation->setSpeed(50);
        } else if (state == QTextToSpeech::Ready) {
            animation->setSpeed(100);
            QTimer::singleShot(500, ear, &VoiceEar::startListening);
        }
    });

    // 2. VISUALS
    connect(ear, &VoiceEar::listeningStateChanged, this, [this](bool rec){
        if(rec) {
            reactorLabel->setStyleSheet("border: 4px solid #00FFFF; border-radius: 125px;");
            animation->setSpeed(200);
        } else {
            reactorLabel->setStyleSheet("");
            animation->setSpeed(100);
        }
    });

    // 3. HEARD COMMAND
    connect(ear, &VoiceEar::heardCommand, this, [this](const QString &text){
        QString clean = text.trimmed();
        if (clean.length() < 2) return;
        if (clean.contains("[silence]")) return;

        // Rate Limit
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - lastRequestTime < 2000) return;
        lastRequestTime = now;

        qDebug() << "🎤 HEARD:" << clean;
        reactorLabel->setStyleSheet("border: 4px solid #FFA500; border-radius: 125px;");
        brain->sendMessage(clean);
    });

    // 4. RESPONSES
    connect(brain, &GeminiBrain::responseReceived, this, [this](const QString &t){
        reactorLabel->setStyleSheet("");
        voice->say(t);
    });

    connect(brain, &GeminiBrain::actionTriggered, this, [](const QString &t, const QString &v){
        qDebug() << "⚡ ACTION:" << t << v;
        if(t == "open") ActionEngine::openApplication(v);
        if(t == "web") ActionEngine::openWeb(v);
        if(t == "close") ActionEngine::closeApplication(v);
    });

    QTimer::singleShot(2000, [this](){ voice->say("Jarvis Online."); });
}

void Friday::setupUI() {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    reactorLabel = new QLabel(this);
    reactorLabel->setAlignment(Qt::AlignCenter);

    QString gifPath = ":/assets/reactor.gif";
    if (!QFile::exists(gifPath)) gifPath = QCoreApplication::applicationDirPath() + "/assets/reactor.gif";

    animation = new QMovie(gifPath);
    if (animation->isValid()) {
        reactorLabel->setMovie(animation);
        animation->start();
    } else {
        reactorLabel->setText("GIF ERROR");
        reactorLabel->setStyleSheet("color: red;");
    }
    setCentralWidget(reactorLabel);
    resize(250, 250);
}

// --- MOUSE EVENTS ---
void Friday::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void Friday::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
    }
}

// --- CONTEXT MENU (RIGHT CLICK) ---
void Friday::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    menu.setStyleSheet("QMenu { background: #222; color: #0FF; border: 1px solid #0FF; }");

    // OPTION 1: Change API Key
    menu.addAction("Change API Key", [this](){
        bool ok;
        QString text = QInputDialog::getText(this, "API Key",
                                             "Enter new OpenAI Key:",
                                             QLineEdit::Normal, "", &ok);
        if (ok && !text.isEmpty()) {
            QSettings settings("FridayCorp", "FridayAssistant");
            settings.setValue("openai_key", text.trimmed());
            brain->setApiKey(text.trimmed());
            voice->say("Key updated.");
        }
    });

    // OPTION 2: Reset/Clear Key (For Testing)
    menu.addAction("Clear Key (Reset)", [this](){
        QSettings settings("FridayCorp", "FridayAssistant");
        settings.remove("openai_key");
        voice->say("Key cleared. Restart me.");
        QTimer::singleShot(2000, qApp, &QCoreApplication::quit);
    });

    menu.addSeparator();
    menu.addAction("Exit", qApp, &QCoreApplication::quit);

    menu.exec(event->globalPos());
}
