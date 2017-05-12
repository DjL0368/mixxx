#include <QDesktopWidget>
#include <QWheelEvent>

#include "library/dlgcoverartfullsize.h"
#include "library/coverartutils.h"
#include "library/coverartcache.h"

DlgCoverArtFullSize::DlgCoverArtFullSize(QWidget* parent, BaseTrackPlayer* pPlayer)
        : QDialog(parent),
          m_pPlayer(pPlayer),
          m_pCoverMenu(new WCoverArtMenu(this)) {
    CoverArtCache* pCache = CoverArtCache::instance();
    if (pCache != nullptr) {
        connect(pCache, SIGNAL(coverFound(const QObject*,
                                          const CoverInfo&, QPixmap, bool)),
                this, SLOT(slotCoverFound(const QObject*,
                                          const CoverInfo&, QPixmap, bool)));
    }

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotCoverMenu(QPoint)));
    connect(m_pCoverMenu, SIGNAL(coverInfoSelected(const CoverInfo&)),
            this, SLOT(slotCoverInfoSelected(const CoverInfo&)));
    connect(m_pCoverMenu, SIGNAL(reloadCoverArt()),
            this, SLOT(slotReloadCoverArt()));

    if (m_pPlayer != nullptr) {
        connect(pPlayer, SIGNAL(newTrackLoaded(TrackPointer)),
                this, SLOT(slotLoadTrack(TrackPointer)));
    }

    setupUi(this);
}

DlgCoverArtFullSize::~DlgCoverArtFullSize() {
    delete m_pCoverMenu;
}

void DlgCoverArtFullSize::init(TrackPointer pTrack) {
    if (pTrack == nullptr) {
        return;
    }
    slotLoadTrack(pTrack);

    show();
    raise();
    activateWindow();
}

void DlgCoverArtFullSize::slotLoadTrack(TrackPointer pTrack) {
    if (m_pLoadedTrack != nullptr) {
        disconnect(m_pLoadedTrack.get(), SIGNAL(coverArtUpdated()),
                   this, SLOT(slotTrackCoverArtUpdated()));
    }
    m_pLoadedTrack = pTrack;
    if (m_pLoadedTrack != nullptr) {
        QString windowTitle;
        const QString albumArtist = m_pLoadedTrack->getAlbumArtist();
        const QString artist = m_pLoadedTrack->getArtist();
        const QString album = m_pLoadedTrack->getAlbum();
        const QString year = m_pLoadedTrack->getYear();
        if (!albumArtist.isEmpty()) {
            windowTitle = albumArtist;
        } else if (!artist.isEmpty()) {
            windowTitle += artist;
        }
        if (!album.isEmpty()) {
            if (!windowTitle.isEmpty()) {
                windowTitle += " - ";
            }
            windowTitle += album;
        }
        if (!year.isEmpty()) {
            if (!windowTitle.isEmpty()) {
                windowTitle += " ";
            }
            windowTitle += QString("(%1)").arg(year);
        }
        setWindowTitle(windowTitle);

        connect(m_pLoadedTrack.get(), SIGNAL(coverArtUpdated()),
                this, SLOT(slotTrackCoverArtUpdated()));
    }

    slotTrackCoverArtUpdated();
}

void DlgCoverArtFullSize::slotTrackCoverArtUpdated() {
    if (m_pLoadedTrack != nullptr) {
        CoverArtCache::requestCover(*m_pLoadedTrack, this);
    }
}

void DlgCoverArtFullSize::slotCoverFound(const QObject* pRequestor,
                                         const CoverInfo& info, QPixmap pixmap,
                                         bool fromCache) {
    Q_UNUSED(info);
    Q_UNUSED(fromCache);

    if (pRequestor == this && m_pLoadedTrack != nullptr &&
            m_pLoadedTrack->getCoverHash() == info.hash) {
        // qDebug() << "DlgCoverArtFullSize::slotCoverFound" << pRequestor << info
        //          << pixmap.size();
        m_pixmap = pixmap;
        move(QApplication::desktop()->screenGeometry().center() - rect().center());
        if (m_pixmap.isNull()) {
            close();
        } else {
            // Scale down dialog to screen size if the pixmap is larger than the screen
            QSize dialogSize = m_pixmap.size();
            const QSize availableScreenSpace =
                QApplication::desktop()->availableGeometry().size();
            if (dialogSize.height() > availableScreenSpace.height()) {
                dialogSize.scale(dialogSize.width(), availableScreenSpace.height(),
                                 Qt::KeepAspectRatio);
            } else if (dialogSize.width() > availableScreenSpace.width()) {
                dialogSize.scale(availableScreenSpace.width(), dialogSize.height(),
                                 Qt::KeepAspectRatio);
            }
            resize(dialogSize);
        }
    }
}

// slots to handle signals from the context menu
void DlgCoverArtFullSize::slotReloadCoverArt() {
    if (m_pLoadedTrack != nullptr) {
        CoverInfo coverInfo =
                CoverArtUtils::guessCoverInfo(*m_pLoadedTrack);
        slotCoverInfoSelected(coverInfo);
    }
}

void DlgCoverArtFullSize::slotCoverInfoSelected(const CoverInfo& coverInfo) {
    qDebug() << "DlgCoverArtFullSize::slotCoverInfoSelected" << coverInfo;
    if (m_pLoadedTrack != nullptr) {
        m_pLoadedTrack->setCoverInfo(coverInfo);
    }
}

void DlgCoverArtFullSize::mousePressEvent(QMouseEvent* event) {
    Q_UNUSED(event);

    if (m_pCoverMenu->isVisible()) {
        return;
    }

    if (event->button() == Qt::LeftButton && isVisible()) {
        close();
    }
}

void DlgCoverArtFullSize::slotCoverMenu(const QPoint& pos) {
    m_pCoverMenu->popup(mapToGlobal(pos));
}

void DlgCoverArtFullSize::resizeEvent(QResizeEvent* event) {
    Q_UNUSED(event);
    if (m_pixmap.isNull()) {
        return;
    }
    qDebug() << "DlgCoverArtFullSize::resizeEvent" << size();
    QPixmap resizedPixmap = m_pixmap.scaled(size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation);
    coverArt->setPixmap(resizedPixmap);
}

void DlgCoverArtFullSize::wheelEvent(QWheelEvent* event) {
    QPoint oldOrigin = frameGeometry().topLeft();
    int oldWidth = frameGeometry().width();
    int oldHeight = frameGeometry().height();
    int newWidth = oldWidth + (0.2 * event->delta());
    int newHeight = oldHeight + (0.2 * event->delta());
    QSize newSize = size();
    newSize.scale(newWidth, newHeight, Qt::KeepAspectRatio);
    resize(newSize);
    QPoint newOrigin = QPoint(oldOrigin.x() + (oldWidth - frameGeometry().width()) / 2,
                              oldOrigin.y() + (oldHeight - frameGeometry().height()) / 2 );
    move(newOrigin);
    event->accept();
}
