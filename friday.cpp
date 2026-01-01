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

Friday::Friday(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    voice = new QTextToSpeech(this);
    brain = new GeminiBrain(this);
    ear = new VoiceEar(this);

    QString finalKey = "";

    // 1. Check System Environment Variable (Good for Developers)
    // You set this in Windows Settings -> Env Vars -> "OPENAI_API_KEY"
    finalKey = qEnvironmentVariable("OPENAI_API_KEY");

    if (!finalKey.isEmpty()) {
        qDebug() << "🔑 Found API Key in Environment Variables.";
    }
    // 1.5 Check for local secrets.txt file (Deployment Override)
    if (finalKey.isEmpty()) {
        QFile file(QCoreApplication::applicationDirPath() + "/secrets.txt");
        if (file.open(QIODevice::ReadOnly)) {
            finalKey = QString::fromUtf8(file.readAll()).trimmed();
            qDebug() << "🔑 Found API Key in secrets.txt";
        }
    }
    else {
        // 2. Check Saved Settings (Good for Users)
        QSettings settings("FridayCorp", "FridayAssistant");
        finalKey = settings.value("openai_key").toString();

        // 3. If still empty, Ask the User (First Run)
        if (finalKey.isEmpty()) {
            bool ok;
            QString text = QInputDialog::getText(this, "Friday Setup",
                                                 "Enter OpenAI API Key (sk-...):",
                                                 QLineEdit::Normal, "", &ok);
            if (ok && !text.isEmpty()) {
                finalKey = text.trimmed();
                settings.setValue("openai_key", finalKey); // Save for next time
            }
        }
    }

    // 4. Send Key to Brain
    if (finalKey.isEmpty()) {
        voice->say("I cannot function without a key.");
    } else {
        brain->setApiKey(finalKey);
    }

    // 1. PREVENT ECHO
    connect(voice, &QTextToSpeech::stateChanged, this, [this](QTextToSpeech::State state){
        if (state == QTextToSpeech::Speaking) {
            ear->stopListening();
            animation->setSpeed(50);
        } else if (state == QTextToSpeech::Ready) {
            animation->setSpeed(100);
            QTimer::singleShot(500, ear, &VoiceEar::startListening);
        }
    });

    // 2. VISUALS (Updated for Sleep Mode)
    connect(ear, &VoiceEar::listeningStateChanged, this, [this](bool rec){
        // If Sleeping, stay Grey. Don't flash Red.
        if (isSleeping) {
            reactorLabel->setStyleSheet("border: 4px solid #555555; border-radius: 125px;"); // Grey
            return;
        }

        if(rec) {
            reactorLabel->setStyleSheet("border: 4px solid #00FFFF; border-radius: 125px;"); // Cyan
            animation->setSpeed(200);
        } else {
            reactorLabel->setStyleSheet("");
            animation->setSpeed(100);
        }
    });

    // 3. HEARD COMMAND (LOGIC UPGRADE)
    connect(ear, &VoiceEar::heardCommand, this, [this](const QString &text){
        QString clean = text.trimmed();
        QString t = clean.toLower();

        // --- FILTER GARBAGE ---
        if (clean.length() < 2) return;
        if (clean.contains("[silence]")) return;

        qDebug() << "🎤 HEARD:" << clean;

        // --- COMMAND 1: SHUT DOWN (Close App) ---
        if (t.contains("shut down") || t.contains("goodbye") || t.contains("exit")) {
            voice->say("Goodbye, sir. Powering down.");
            reactorLabel->setStyleSheet("border: 4px solid #000000; border-radius: 125px;");
            // Close app after speaking (2 seconds)
            QTimer::singleShot(2000, qApp, &QCoreApplication::quit);
            return;
        }

        // --- COMMAND 2: WAKE UP ---
        if (t.contains("wake up") || t.contains("friday")) {
            if (isSleeping) {
                isSleeping = false;
                voice->say("Systems restored. I am listening.");
                reactorLabel->setStyleSheet("border: 4px solid #00FFFF; border-radius: 125px;"); // Cyan
                return;
            }
        }

        // --- IF SLEEPING, IGNORE EVERYTHING ELSE ---
        if (isSleeping) {
            qDebug() << "💤 Sleeping... Ignoring command.";
            return;
        }

        // --- COMMAND 3: GO TO SLEEP ---
        if (t.contains("go to sleep") || t.contains("sleep mode") || t.contains("stop listening")) {
            isSleeping = true;
            voice->say("Entering standby mode.");
            reactorLabel->setStyleSheet("border: 4px solid #555555; border-radius: 125px;"); // Grey Ring
            return;
        }

        // --- NORMAL PROCESSING (Only if Awake) ---
        // Rate Limit
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - lastRequestTime < 2000) return;
        lastRequestTime = now;

        reactorLabel->setStyleSheet("border: 4px solid #FFA500; border-radius: 125px;"); // Orange = Thinking
        brain->sendMessage(clean);
    });

    connect(brain, &GeminiBrain::actionTriggered, this, [](const QString &t, const QString &v){
        qDebug() << "⚡ ACTION TYPE:" << t << "VALUE:" << v;

        if(t == "open") ActionEngine::openApplication(v);
        if(t == "web")  ActionEngine::openWeb(v);

        // --- NEW: CLOSE ACTION ---
        if(t == "close") {
            ActionEngine::closeApplication(v);
        }
    });

    connect(brain, &GeminiBrain::responseReceived, this, [this](const QString &t){
        reactorLabel->setStyleSheet("");
        voice->say(t);
    });

    connect(brain, &GeminiBrain::actionTriggered, this, [](const QString &t, const QString &v){
        if(t == "app") ActionEngine::openApplication(v);
        if(t == "web") ActionEngine::openWeb(v);
    });

    QTimer::singleShot(2000, [this](){ voice->say("Jarvis Online."); });
}

// ... (Keep setupUI and mouse events exactly the same as before) ...
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
