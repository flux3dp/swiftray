#ifndef DXFREADER_H
#define DXFREADER_H

#include <QList.h>
#include <QString.h>
#include <layer.h>

#include "parser/dxf_rs/engine/rs_entitycontainer.h"

class DXFReader
{
public:
    enum ReadType {
        InSignleLayer,
        ByLayers,
        ByColors
    };
    DXFReader();
    void openFile(const QString &file_name, QList<LayerPtr> &dxf_layers, ReadType read_type);

private:
    int read_type_;
    QList<LayerPtr> dxf_layers_;

    LayerPtr findLayer(QString layer_name, QColor color);
    void handleEntityContainer(RS_EntityContainer* container);
    void handleEntityCircle(RS_Circle* entity);
    void handleEntityArc(RS_Arc* entity);
    void handleEntityLine(RS_Line* entity);
};

#endif // DXFREADER_H
