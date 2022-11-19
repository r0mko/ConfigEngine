#ifndef FONTINFO_H
#define FONTINFO_H

#include <QObject>

class FontInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString family READ family WRITE setFamily NOTIFY familyChanged)
    Q_PROPERTY(int size READ size WRITE setSize NOTIFY sizeChanged)

public:
    Q_INVOKABLE FontInfo(QObject *parent = nullptr);

    QString name() const;
    QString family();
    int size() const;

    void setName(const QString& name);
    void setFamily(const QString& family);
    void setSize(int size);

    void setUseJapaneseFont(bool newValue);
    bool useJapaneseFont() const;

signals:
    void nameChanged();
    void familyChanged();
    void sizeChanged();

private:
    QString m_name;
    QString m_family;
    QString m_localizedFamily;
    int m_size;

    bool m_useJapaneseFont = false;
};

Q_DECLARE_METATYPE(FontInfo*);
#endif // FONTINFO_H
