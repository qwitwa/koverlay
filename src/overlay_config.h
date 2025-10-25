#pragma once
#include <QObject>
#include <QString>

class OverlayConfig : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString textFile READ textFile WRITE setTextFile NOTIFY textFileChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(QString textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(bool bold READ bold WRITE setBold NOTIFY boldChanged)
    Q_PROPERTY(double panelOpacity READ panelOpacity WRITE setPanelOpacity NOTIFY panelOpacityChanged)

    // Positioning
    Q_PROPERTY(QString position READ position WRITE setPosition NOTIFY positionChanged)
    // top-left|top-right|bottom-left|bottom-right|custom
    Q_PROPERTY(int marginTop READ marginTop WRITE setMarginTop NOTIFY positionChanged)
    Q_PROPERTY(int marginRight READ marginRight WRITE setMarginRight NOTIFY positionChanged)
    Q_PROPERTY(int marginBottom READ marginBottom WRITE setMarginBottom NOTIFY positionChanged)
    Q_PROPERTY(int marginLeft READ marginLeft WRITE setMarginLeft NOTIFY positionChanged)
    Q_PROPERTY(int x READ x WRITE setX NOTIFY positionChanged) // custom only
    Q_PROPERTY(int y READ y WRITE setY NOTIFY positionChanged) // custom only
public:
    explicit OverlayConfig(QObject *parent = nullptr) : QObject(parent) {
    }

    // text / textFile
    const QString &text() const { return text_; }

    void setText(QString v) {
        if (text_ == v) return;
        text_ = std::move(v);
        emit textChanged();
    }

    const QString &textFile() const { return textFile_; }

    void setTextFile(QString v) {
        if (textFile_ == v) return;
        textFile_ = std::move(v);
        emit textFileChanged();
    }

    // font family
    const QString &fontFamily() const { return fontFamily_; }

    void setFontFamily(QString v) {
        if (fontFamily_ == v) return;
        fontFamily_ = std::move(v);
        emit fontFamilyChanged();
    }

    // font size
    int fontSize() const { return fontSize_; }

    void setFontSize(int v) {
        if (fontSize_ == v) return;
        fontSize_ = v;
        emit fontSizeChanged();
    }

    // text color
    const QString &textColor() const { return textColor_; }

    void setTextColor(QString v) {
        if (textColor_ == v) return;
        textColor_ = std::move(v);
        emit textColorChanged();
    }

    // bold
    bool bold() const { return bold_; }

    void setBold(bool v) {
        if (bold_ == v) return;
        bold_ = v;
        emit boldChanged();
    }

    // panel opacity
    double panelOpacity() const { return panelOpacity_; }

    void setPanelOpacity(double v) {
        if (qFuzzyCompare(panelOpacity_, v)) return;
        panelOpacity_ = v;
        emit panelOpacityChanged();
    }

    // Positioning (QString normalized lower-case)
    const QString &position() const { return position_; }

    void setPosition(QString v) {
        QString n = v.trimmed().toLower();
        if (n.isEmpty()) n = QStringLiteral("top-right");
        if (position_ == n) return;
        position_ = std::move(n);
        emit positionChanged();
    }

    int marginTop() const { return marginTop_; }
    int marginRight() const { return marginRight_; }
    int marginBottom() const { return marginBottom_; }
    int marginLeft() const { return marginLeft_; }

    void setMarginTop(int v) {
        if (marginTop_ == v) return;
        marginTop_ = v;
        emit positionChanged();
    }

    void setMarginRight(int v) {
        if (marginRight_ == v) return;
        marginRight_ = v;
        emit positionChanged();
    }

    void setMarginBottom(int v) {
        if (marginBottom_ == v) return;
        marginBottom_ = v;
        emit positionChanged();
    }

    void setMarginLeft(int v) {
        if (marginLeft_ == v) return;
        marginLeft_ = v;
        emit positionChanged();
    }

    int x() const { return x_; }
    int y() const { return y_; }

    void setX(int v) {
        if (x_ == v) return;
        x_ = v;
        emit positionChanged();
    }

    void setY(int v) {
        if (y_ == v) return;
        y_ = v;
        emit positionChanged();
    }

signals:
    void textChanged();

    void textFileChanged();

    void fontFamilyChanged();

    void fontSizeChanged();

    void textColorChanged();

    void boldChanged();

    void panelOpacityChanged();

    void positionChanged();

private:
    // text
    QString text_;
    QString textFile_;

    // style
    QString fontFamily_;
    int fontSize_ = 28;
    QString textColor_ = QStringLiteral("#FFFFFF");
    bool bold_ = true;
    double panelOpacity_ = 0.35; // matches #59 alpha (~89/255)

    // position
    QString position_ = QStringLiteral("top-right");
    int marginTop_ = 16;
    int marginRight_ = 16;
    int marginBottom_ = 16;
    int marginLeft_ = 16;
    int x_ = 0;
    int y_ = 0;
};
