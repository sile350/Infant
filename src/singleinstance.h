#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H

class SingleInstance {
public:
    SingleInstance();
    ~SingleInstance();

    bool isPrimary() const;

private:
    class Private;
    Private *m_private = nullptr;
};

#endif
