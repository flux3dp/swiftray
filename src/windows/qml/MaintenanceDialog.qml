import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2
import QtQuick.Shapes 1.14

Rectangle {
    id: root
    width: 280
    height: 314
    color: is_dark_mode ? "#333333" : "#F0F0F0"
    signal home()
    signal laser()
    signal laserPulse()
    signal moveRelatively(int dir, int level)
    signal moveToCorner(int corner)
    signal moveToEdge(int dir)

    Rectangle {
        x: 10
        y: 10
        width: 538
        height: 538
        color: "transparent"
        transform: Scale { origin.x: 0; origin.y: 0; xScale: 260/538; yScale: 260/538 }

        Shape {
            id: pathSvg
            vendorExtensionsEnabled: false
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
                right: parent.right
            }

            visible: true

            ShapePath {
                strokeWidth: 1
                fillColor: "#555249"
                strokeColor: "#555249"
                startX: 0; startY: 0
                PathSvg {
                    path: "M124.5,121.3l36.6,36.8l36.7,36.9l36.8,37c18.4-14,44-14,62.4,0l36.8-37l36.7-36.9l36.6-36.8l36.6-36.8
                c-0.2-0.2-0.3-0.3-0.5-0.5l0.5-0.5c0,0,0,0,0,0l0,0c-0.2-0.2-0.3-0.3-0.5-0.5l0.5-0.5c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0
                c-0.2-0.2-0.3-0.3-0.5-0.5l0.5-0.5c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c-0.2-0.2-0.3-0.3-0.5-0.5l0.5-0.5c0,0,0,0,0,0
                l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c-0.2-0.2-0.3-0.3-0.5-0.5l0.5-0.5c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0
                c-0.2-0.2-0.3-0.3-0.5-0.5l0.5-0.5c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c-0.2-0.2-0.3-0.3-0.5-0.5l0.5-0.5c0,0,0,0,0,0l0,0
                c-0.2-0.2-0.3-0.3-0.5-0.5l0.5-0.5c-99.8-95-256.1-95-355.9,0l0.5,0.5c-0.2,0.2-0.3,0.3-0.5,0.5l0,0c0,0,0,0,0,0l0.5,0.5
                c-0.2,0.2-0.3,0.3-0.5,0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0.5,0.5c-0.2,0.2-0.3,0.3-0.5,0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0
                c0,0,0,0,0,0l0.5,0.5c-0.2,0.2-0.3,0.3-0.5,0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0.5,0.5c-0.2,0.2-0.3,0.3-0.5,0.5
                l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0.5,0.5c-0.2,0.2-0.3,0.3-0.5,0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0.5,0.5
                c-0.2,0.2-0.3,0.3-0.5,0.5l0,0c0,0,0,0,0,0l0.5,0.5c-0.2,0.2-0.3,0.3-0.5,0.5L124.5,121.3z"
                }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#555249"
                strokeColor: "#555249"
                startX: 0; startY: 0
                PathSvg { path: "M151,378.7l36.7-36.9l36.8-37c-0.1-0.2-0.3-0.4-0.4-0.6l0.4-0.4c0,0,0,0,0,0l0,0c-0.1-0.2-0.3-0.4-0.4-0.6
                l0.4-0.4c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c-0.1-0.2-0.3-0.4-0.4-0.5l0.4-0.4c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0
                c-0.1-0.2-0.3-0.4-0.4-0.5l0.4-0.4c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c-0.1-0.2-0.3-0.4-0.4-0.5l0.4-0.4c0,0,0,0,0,0
                l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c-0.1-0.2-0.3-0.4-0.4-0.5l0.4-0.4c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c-0.1-0.2-0.3-0.4-0.4-0.6
                l0.4-0.4c0,0,0,0,0,0l0,0c-0.1-0.2-0.3-0.4-0.4-0.6l0.4-0.4c-6.1-8.1-9.5-17.6-10.3-27.3c0.8-9.7,4.2-19.2,10.3-27.3l-0.4-0.4
                c0.1-0.2,0.3-0.4,0.4-0.6l0,0c0,0,0,0,0,0l-0.4-0.4c0.1-0.2,0.3-0.4,0.4-0.6l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l-0.4-0.4
                c0.1-0.2,0.3-0.4,0.4-0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l-0.4-0.4c0.1-0.2,0.3-0.4,0.4-0.5l0,0c0,0,0,0,0,0l0,0
                c0,0,0,0,0,0l0,0c0,0,0,0,0,0l-0.4-0.4c0.1-0.2,0.3-0.4,0.4-0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l-0.4-0.4
                c0.1-0.2,0.3-0.4,0.4-0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l-0.4-0.4c0.1-0.2,0.3-0.4,0.4-0.6l0,0c0,0,0,0,0,0l-0.4-0.4
                c0.1-0.2,0.3-0.4,0.4-0.6l-36.8-37L151,160.2l-36.6-36.8L77.8,86.5c-47.4,50.3-71,114.8-70.9,179.4c0,0.3,0,0.7,0,1c0,0,0,0,0,0
                c0,0.3,0,0.6,0,1c0,0,0,0,0,0c0,0,0,0,0,0c0,0.3,0,0.6,0,0.9c0,0,0,0,0,0c0,0,0,0,0,0c0,0,0,0,0,0c0,0.3,0,0.6,0,0.9c0,0,0,0,0,0
                c0,0,0,0,0,0c0,0,0,0,0,0c0,0.3,0,0.6,0,0.9c0,0,0,0,0,0c0,0,0,0,0,0c0,0.3,0,0.6,0,1c0,0,0,0,0,0c0,0.3,0,0.7,0,1
                c-0.1,64.5,23.5,129.1,70.9,179.4l36.6-36.8L151,378.7z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#555249"
                strokeColor: "#555249"
                startX: 0; startY: 0
                PathSvg { path: "M453.9,86.5l-36.6,36.8l-36.6,36.8L343.9,197l-36.8,37c0.1,0.2,0.3,0.4,0.4,0.6l-0.4,0.4c0,0,0,0,0,0l0,0
                c0.1,0.2,0.3,0.4,0.4,0.6l-0.4,0.4c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0.1,0.2,0.3,0.4,0.4,0.5l-0.4,0.4c0,0,0,0,0,0l0,0c0,0,0,0,0,0
                l0,0c0,0,0,0,0,0l0,0c0.1,0.2,0.3,0.4,0.4,0.5l-0.4,0.4c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0.1,0.2,0.3,0.4,0.4,0.5
                l-0.4,0.4c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0.1,0.2,0.3,0.4,0.4,0.5l-0.4,0.4c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0
                c0.1,0.2,0.3,0.4,0.4,0.6l-0.4,0.4c0,0,0,0,0,0l0,0c0.1,0.2,0.3,0.4,0.4,0.6l-0.4,0.4c6.1,8.1,9.5,17.6,10.3,27.3
                c-0.8,9.7-4.2,19.2-10.3,27.3l0.4,0.4c-0.1,0.2-0.3,0.4-0.4,0.6l0,0c0,0,0,0,0,0l0.4,0.4c-0.1,0.2-0.3,0.4-0.4,0.6l0,0c0,0,0,0,0,0
                l0,0c0,0,0,0,0,0l0.4,0.4c-0.1,0.2-0.3,0.4-0.4,0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0.4,0.4
                c-0.1,0.2-0.3,0.4-0.4,0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0.4,0.4c-0.1,0.2-0.3,0.4-0.4,0.5l0,0c0,0,0,0,0,0l0,0
                c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0.4,0.4c-0.1,0.2-0.3,0.4-0.4,0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0.4,0.4c-0.1,0.2-0.3,0.4-0.4,0.6
                l0,0c0,0,0,0,0,0l0.4,0.4c-0.1,0.2-0.3,0.4-0.4,0.6l36.8,37l36.7,36.9l36.6,36.8l36.6,36.8c47.4-50.3,71-114.8,70.9-179.4
                c0-0.3,0-0.7,0-1c0,0,0,0,0,0c0-0.3,0-0.6,0-1c0,0,0,0,0,0c0,0,0,0,0,0c0-0.3,0-0.6,0-0.9c0,0,0,0,0,0c0,0,0,0,0,0c0,0,0,0,0,0
                c0-0.3,0-0.6,0-0.9c0,0,0,0,0,0c0,0,0,0,0,0c0,0,0,0,0,0c0-0.3,0-0.6,0-0.9c0,0,0,0,0,0c0,0,0,0,0,0c0-0.3,0-0.6,0-1c0,0,0,0,0,0
                c0-0.3,0-0.7,0-1C524.9,201.4,501.3,136.8,453.9,86.5z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#555249"
                strokeColor: "#555249"
                startX: 0; startY: 0
                PathSvg { path: "M407.1,417.5l-36.6-36.8l-36.7-36.9l-36.8-37c-18.4,14-44,14-62.4,0l-36.8,37l-36.7,36.9l-36.6,36.8
                l-36.6,36.8c0.2,0.2,0.3,0.3,0.5,0.5l-0.5,0.5c0,0,0,0,0,0l0,0c0.2,0.2,0.3,0.3,0.5,0.5l-0.5,0.5c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0
                c0.2,0.2,0.3,0.3,0.5,0.5l-0.5,0.5c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0.2,0.2,0.3,0.3,0.5,0.5l-0.5,0.5c0,0,0,0,0,0
                l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0.2,0.2,0.3,0.3,0.5,0.5l-0.5,0.5c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0
                c0.2,0.2,0.3,0.3,0.5,0.5l-0.5,0.5c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0.2,0.2,0.3,0.3,0.5,0.5l-0.5,0.5c0,0,0,0,0,0l0,0
                c0.2,0.2,0.3,0.3,0.5,0.5l-0.5,0.5c99.8,95,256.1,95,355.9,0l-0.5-0.5c0.2-0.2,0.3-0.3,0.5-0.5l0,0c0,0,0,0,0,0l-0.5-0.5
                c0.2-0.2,0.3-0.3,0.5-0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l-0.5-0.5c0.2-0.2,0.3-0.3,0.5-0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0
                c0,0,0,0,0,0l-0.5-0.5c0.2-0.2,0.3-0.3,0.5-0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l-0.5-0.5c0.2-0.2,0.3-0.3,0.5-0.5
                l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l-0.5-0.5c0.2-0.2,0.3-0.3,0.5-0.5l0,0c0,0,0,0,0,0l0,0c0,0,0,0,0,0l-0.5-0.5
                c0.2-0.2,0.3-0.3,0.5-0.5l0,0c0,0,0,0,0,0l-0.5-0.5c0.2-0.2,0.3-0.3,0.5-0.5L407.1,417.5z" }
            }
        }
        // Home Button
        Shape {
            containsMode: Shape.FillContains

            ShapePath {
                strokeWidth: 1
                fillColor: "#555249"
                strokeColor: "#555249"
                startX: 0; startY: 0
                PathSvg { path: "M302.6,232.5c-20.3-20.3-53.3-20.3-73.6,0c-20.3,20.3-20.3,53.3,0,73.6c20.3,20.3,53.3,20.3,73.6,0C323,285.8,323,252.9,302.6,232.5z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#444138"
                strokeColor: "#444138"
                startX: 0; startY: 0
                PathSvg { path: "M302.6,227.5c-20.3-20.3-53.3-20.3-73.6,0c-20.3,20.3-20.3,53.3,0,73.6c20.3,20.3,53.3,20.3,73.6,0C323,280.8,323,247.9,302.6,227.5z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 0; startY: 0
                PathSvg { path: "M250.5,272.5h-3.1v-6.8h-5.2v6.8H239v-16.3h3.2v7h5.2v-7h3.1V272.5z" }
                PathSvg { path: "M264.6,266.6c0,1.9-0.5,3.4-1.6,4.5c-1.1,1.1-2.5,1.6-4.3,1.6c-1.8,0-3.2-0.5-4.3-1.6
                            c-1.1-1.1-1.6-2.6-1.6-4.5v-4.5c0-1.9,0.5-3.4,1.6-4.5c1.1-1.1,2.5-1.6,4.3-1.6c1.8,0,3.2,0.5,4.3,1.6c1.1,1.1,1.6,2.6,1.6,4.5
                            V266.6z M261.5,262c0-1.2-0.2-2.1-0.7-2.7c-0.5-0.6-1.2-0.9-2.1-0.9c-0.9,0-1.6,0.3-2.1,0.9c-0.5,0.6-0.7,1.5-0.7,2.7v4.6
                            c0,1.2,0.2,2.1,0.7,2.7c0.5,0.6,1.2,0.9,2.1,0.9c0.9,0,1.6-0.3,2.1-0.9c0.5-0.6,0.7-1.5,0.7-2.7V262z" }
                PathSvg { path: "M273.9,268.3L273.9,268.3l3.1-12.1h4.1v16.3H278v-10.2l-0.1,0l-2.9,10.3h-2.1l-2.9-10.2l-0.1,0v10.2h-3.1v-16.3h4.1L273.9,268.3z" }
                PathSvg { path: "M291.7,265.3h-5v4.7h6v2.5h-9.1v-16.3h9.1v2.5h-5.9v4.2h5V265.3z" }
            }

            TapHandler {
                onTapped: home()
            }
        }

        // Top min
        Shape {
            containsMode: Shape.FillContains
            ShapePath {
                strokeWidth: 1
                fillColor: "#B3B0AB"
                strokeColor: "#B3B0AB"
                startX: 0; startY: 0
                PathSvg { path: "M123.8,112.1c79.9-74.7,204.1-74.7,284,0l36.8-36.8c-100.3-95-257.4-95-357.7,0L123.8,112.1z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 266; startY: 25.8
                PathLine { x: 266; y: 25.8 }
                PathLine { x: 253.9; y: 43.2 }
                PathLine { x: 278.2; y: 43.2 }
                PathLine { x: 266; y: 25.8 }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 253.9; startY: 20.7
                PathLine { x: 253.9; y: 20.7 }
                PathLine { x: 253.9; y: 23.7 }
                PathLine { x: 278.2; y: 23.7 }
                PathLine { x: 278.2; y: 20.7 }
                PathLine { x: 253.9; y: 20.7 }
            }

            TapHandler {
                onTapped: moveToEdge(1)
            }
        }

        // Left min
        Shape {
            containsMode: Shape.FillContains

            ShapePath {
                strokeWidth: 1
                fillColor: "#B3B0AB"
                strokeColor: "#B3B0AB"
                startX: 0; startY: 0
                PathSvg { path: "M113.6,406.3c-74.7-79.9-74.7-204.1,0-284L76.8,85.5c-95,100.3-95,257.4,0,357.7L113.6,406.3z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 25.9; startY: 263.1
                PathLine { x: 25.9; y: 263.1 }
                PathLine { x: 43.3; y: 275.2 }
                PathLine { x: 43.3; y: 250.9 }
                PathLine { x: 25.9; y: 263.1 }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 20.8; startY: 250.9
                PathLine { x: 20.8; y: 250.9 }
                PathLine { x: 20.8; y: 275.2 }
                PathLine { x: 23.8; y: 275.2 }
                PathLine { x: 23.8; y: 250.9 }
                PathLine { x: 20.8; y: 250.9 }
            }

            TapHandler {
                onTapped: moveToEdge(2)
            }
        }

        // Bottom Max
        Shape {
            containsMode: Shape.FillContains

            ShapePath {
                strokeWidth: 1
                fillColor: "#B3B0AB"
                strokeColor: "#B3B0AB"
                startX: 0; startY: 0
                PathSvg { path: "M407.8,416.5c-79.9,74.7-204.1,74.7-284,0L87,453.3c100.3,95,257.4,95,357.7,0L407.8,416.5z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 265.9; startY: 503.1
                PathLine { x: 265.9; y: 503.1 }
                PathLine { x: 278; y: 485.7 }
                PathLine { x: 253.7; y: 485.7 }
                PathLine { x: 265.9; y: 503.1 }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 253.7; startY: 505.2
                PathLine { x: 253.7; y: 505.2 }
                PathLine { x: 253.7; y: 508.2 }
                PathLine { x: 278; y: 508.2 }
                PathLine { x: 278; y: 505.2 }
                PathLine { x: 253.7; y: 505.2 }
            }

            TapHandler {
                onTapped: moveToEdge(3)
            }
        }

        // Right Max
        Shape {
            containsMode: Shape.FillContains

            ShapePath {
                strokeWidth: 1
                fillColor: "#B3B0AB"
                strokeColor: "#B3B0AB"
                startX: 0; startY: 0
                PathSvg { path: "M454.8,85.5L418,122.3c74.7,79.9,74.7,204.1,0,284l36.8,36.8C549.9,342.9,549.9,185.8,454.8,85.5z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 507.5; startY: 263
                PathLine { x: 507.5; y: 263 }
                PathLine { x: 490.1; y: 250.9 }
                PathLine { x: 490.1; y: 275.2 }
                PathLine { x: 507.5; y: 263 }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 509.6; startY: 250.9
                PathLine { x: 509.6; y: 250.9 }
                PathLine { x: 509.6; y: 275.2 }
                PathLine { x: 512.6; y: 275.2 }
                PathLine { x: 512.6; y: 250.9 }
                PathLine { x: 509.6; y: 250.9 }
            }

            TapHandler {
                onTapped: moveToEdge(0)
            }
        }

        // Top 1x
        Shape {
            containsMode: Shape.FillContains

            ShapePath {
                strokeWidth: 1
                fillColor: "#666360"
                strokeColor: "#666360"
                startX: 0; startY: 0
                PathSvg { path: "M297.2,222.8l37-37c-39.1-34.1-97.6-34.1-136.7,0l37,37C253,208.8,278.7,208.8,297.2,222.8z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 265.8; startY: 176.6
                PathLine { x: 265.8; y: 176.6 }
                PathLine { x: 253.7; y: 197.7 }
                PathLine { x: 278; y: 197.7 }
                PathLine { x: 265.8; y: 176.6 }
            }

            TapHandler {
                onTapped: moveRelatively(1, 0)
            }
        }

        // Bottom 1x
        Shape {
            containsMode: Shape.FillContains
            ShapePath {
                strokeWidth: 1
                fillColor: "#666360"
                strokeColor: "#666360"
                startX: 0; startY: 0
                PathSvg { path: "M234.5,305.9l-37,37c39.1,34.1,97.6,34.1,136.7,0l-37-37C278.7,319.9,253,319.9,234.5,305.9z" }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 265.8; startY: 352.1
                PathLine { x: 265.8; y: 352.1 }
                PathLine { x: 278; y: 331 }
                PathLine { x: 253.7; y: 331 }
                PathLine { x: 265.8; y: 352.1 }
            }
            TapHandler {
                onTapped: moveRelatively(3, 0)
            }
        }

        // Left 1x
        Shape {
            containsMode: Shape.FillContains
            ShapePath {
                strokeWidth: 1
                fillColor: "#666360"
                strokeColor: "#666360"
                startX: 0; startY: 0
                PathSvg { path: "M224.3,233l-37-37c-34.1,39.1-34.1,97.6,0,136.7l37-37C210.3,277.2,210.3,251.5,224.3,233z" }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 178.1; startY: 264.3
                PathLine { x: 178.1; y: 264.3 }
                PathLine { x: 199.2; y: 276.5 }
                PathLine { x: 199.2; y: 252.2 }
                PathLine { x: 178.1; y: 264.3 }
            }
            TapHandler {
                onTapped: moveRelatively(2, 0)
            }
        }

        // Right 1x
        Shape {
            containsMode: Shape.FillContains
            ShapePath {
                strokeWidth: 1
                fillColor: "#666360"
                strokeColor: "#666360"
                startX: 0; startY: 0
                PathSvg { path: "M344.3,196l-37,37c14,18.5,14,44.2,0,62.7l37,37C378.5,293.5,378.5,235.1,344.3,196z" }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 353.5; startY: 264.3
                PathLine { x: 353.5; y: 264.3 }
                PathLine { x: 332.5; y: 252.2 }
                PathLine { x: 332.5; y: 276.5 }
                PathLine { x: 353.5; y: 264.3 }
            }
            TapHandler {
                onTapped: moveRelatively(0, 0)
            }
        }

        // Left 2x
        Shape {
            containsMode: Shape.FillContains

            // Button
            ShapePath {
                strokeWidth: 1
                fillColor: "#807C79"
                strokeColor: "#807C79"
                startX: 0; startY: 0
                PathSvg { path: "M187.3,196l-36.9-36.9c-54.4,59.5-54.4,150.8,0,210.4l36.9-36.9C153.2,293.5,153.2,235.1,187.3,196z" }
            }

            // chevron
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 135.7; startY: 264.3
                PathLine { x: 135.7; y: 264.3 }
                PathLine { x: 146.2; y: 276.5 }
                PathLine { x: 146.2; y: 252.2 }
                PathLine { x: 135.7; y: 264.3 }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 125.2; startY: 264.3
                PathLine { x: 125.2; y: 264.3 }
                PathLine { x: 135.7; y: 276.5 }
                PathLine { x: 135.7; y: 252.2 }
                PathLine { x: 125.2; y: 264.3 }
            }

            TapHandler {
                onTapped: moveRelatively(2, 1)
            }
        }

        // Bottom 2x
        Shape {
            containsMode: Shape.FillContains

            // Button
            ShapePath {
                strokeWidth: 1
                fillColor: "#807C79"
                strokeColor: "#807C79"
                startX: 0; startY: 0
                PathSvg { path: "M334.2,342.8c-39.1,34.1-97.6,34.1-136.7,0l-36.9,36.9c59.5,54.4,150.8,54.4,210.4,0L334.2,342.8z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 265.8; startY: 394.5
                PathLine { x: 265.8; y: 394.5 }
                PathLine { x: 278; y: 383.9 }
                PathLine { x: 253.7; y: 383.9 }
                PathLine { x: 265.8; y: 394.5 }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 265.8; startY: 405
                PathLine { x: 265.8; y: 405 }
                PathLine { x: 278; y: 394.5 }
                PathLine { x: 253.7; y: 394.5 }
                PathLine { x: 265.8; y: 405 }
            }

            TapHandler {
                onTapped: moveRelatively(3, 1)
            }
        }

        // Top 2x
        Shape {
            containsMode: Shape.FillContains
            // Button
            ShapePath {
                strokeWidth: 1
                fillColor: "#807C79"
                strokeColor: "#807C79"
                startX: 0; startY: 0
                PathSvg { path: "M197.5,185.8c39.1-34.1,97.6-34.1,136.7,0L371,149c-59.5-54.4-150.8-54.4-210.4,0L197.5,185.8z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 265.8; startY: 134.2
                PathLine { x: 265.8; y: 134.2 }
                PathLine { x: 253.7; y: 144.7 }
                PathLine { x: 278; y: 144.7 }
                PathLine { x: 265.8; y: 134.2 }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 265.8; startY: 123.7
                PathLine { x: 265.8; y: 123.7 }
                PathLine { x: 253.7; y: 134.2 }
                PathLine { x: 278; y: 134.2 }
                PathLine { x: 265.8; y: 123.7 }
            }
            TapHandler {
                onTapped: moveRelatively(1, 1)
            }
        }

        // Right 2x
        Shape {
            containsMode: Shape.FillContains
            ShapePath {
                strokeWidth: 1
                fillColor: "#807C79"
                strokeColor: "#807C79"
                startX: 0; startY: 0
                PathSvg { path: "M344.3,332.7l36.9,36.9c54.4-59.5,54.4-150.8,0-210.4L344.3,196C378.5,235.1,378.5,293.5,344.3,332.7z" }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 395.9; startY: 264.3
                PathLine { x: 395.9; y: 264.3 }
                PathLine { x: 385.4; y: 252.2 }
                PathLine { x: 385.4; y: 276.5 }
                PathLine { x: 395.9; y: 264.3 }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 406.5; startY: 264.3
                PathLine { x: 406.5; y: 264.3 }
                PathLine { x: 395.9; y: 252.2 }
                PathLine { x: 395.9; y: 276.5 }
                PathLine { x: 406.5; y: 264.3 }
            }
            TapHandler {
                onTapped: moveRelatively(0, 1)
            }
        }

        // Top 3x
        Shape {
            containsMode: Shape.FillContains
            ShapePath {
                strokeWidth: 1
                fillColor: "#999693"
                strokeColor: "#999693"
                startX: 0; startY: 0
                PathSvg { path: "M160.6,149c59.5-54.4,150.8-54.4,210.4,0l36.8-36.8c-79.9-74.7-204.1-74.7-284,0L160.6,149z" }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 265.8; startY: 86.8
                PathLine { x: 265.8; y: 86.8 }
                PathLine { x: 253.7; y: 93.8 }
                PathLine { x: 278; y: 93.8 }
                PathLine { x: 265.8; y: 86.8 }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 265.8; startY: 80
                PathLine { x: 265.8; y: 80 }
                PathLine { x: 253.7; y: 87 }
                PathLine { x: 278; y: 87 }
                PathLine { x: 265.8; y: 80 }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 265.8; startY: 73.2
                PathLine { x: 265.8; y: 73.2 }
                PathLine { x: 253.7; y: 80.2 }
                PathLine { x: 278; y: 80.2 }
                PathLine { x: 265.8; y: 73.2 }
            }
            TapHandler {
                onTapped: moveRelatively(1, 2)
            }
        }

        // bottom 3x
        Shape {
            containsMode: Shape.FillContains
            ShapePath {
                strokeWidth: 1
                fillColor: "#999693"
                strokeColor: "#999693"
                startX: 0; startY: 0
                PathSvg { path: "M371,379.7c-59.5,54.4-150.8,54.4-210.4,0l-36.8,36.8c79.9,74.7,204.1,74.7,284,0L371,379.7z" }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 265.8; startY: 441.9
                PathLine { x: 265.8; y: 441.9 }
                PathLine { x: 278; y: 434.9 }
                PathLine { x: 253.7; y: 434.9 }
                PathLine { x: 265.8; y: 441.9 }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 265.8; startY: 448.7
                PathLine { x: 265.8; y: 448.7 }
                PathLine { x: 278; y: 441.7 }
                PathLine { x: 253.7; y: 441.7 }
                PathLine { x: 265.8; y: 448.7 }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 265.8; startY: 455.5
                PathLine { x: 265.8; y: 455.5 }
                PathLine { x: 278; y: 448.5 }
                PathLine { x: 253.7; y: 448.5 }
                PathLine { x: 265.8; y: 455.5 }
            }
            TapHandler {
                onTapped: moveRelatively(3, 2)
            }
        }

        // right 3x
        Shape {
            containsMode: Shape.FillContains
            ShapePath {
                strokeWidth: 1
                fillColor: "#999693"
                strokeColor: "#999693"
                startX: 0; startY: 0
                PathSvg { path: "M418,122.3l-36.8,36.8c54.4,59.5,54.4,150.8,0,210.4l36.8,36.8C492.7,326.4,492.7,202.2,418,122.3z" }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 443.4; startY: 264.3
                PathLine { x: 443.4; y: 264.3 }
                PathLine { x: 436.4; y: 252.2 }
                PathLine { x: 436.4; y: 276.5 }
                PathLine { x: 443.4; y: 264.3 }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 450.2; startY: 264.3
                PathLine { x: 450.2; y: 264.3 }
                PathLine { x: 443.2; y: 252.2 }
                PathLine { x: 443.2; y: 276.5 }
                PathLine { x: 450.2; y: 264.3 }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 456.9; startY: 264.3
                PathLine { x: 456.9; y: 264.3 }
                PathLine { x: 450; y: 252.2 }
                PathLine { x: 450; y: 276.5 }
                PathLine { x: 456.9; y: 264.3 }
            }
            TapHandler {
                onTapped: moveRelatively(0, 2)
            }
        }

        // Left 3x
        Shape {
            containsMode: Shape.FillContains
            ShapePath {
                strokeWidth: 1
                fillColor: "#999693"
                strokeColor: "#999693"
                startX: 0; startY: 0
                PathSvg { path: "M150.5,369.5c-54.4-59.5-54.4-150.8,0-210.4l-36.8-36.8c-74.7,79.9-74.7,204.1,0,284L150.5,369.5z" }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 88.3; startY: 264.3
                PathLine { x: 88.3; y: 264.3 }
                PathLine { x: 95.3; y: 276.5 }
                PathLine { x: 95.3; y: 252.2 }
                PathLine { x: 88.3; y: 264.3 }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 81.5; startY: 264.3
                PathLine { x: 81.5; y: 264.3 }
                PathLine { x: 88.5; y: 276.5 }
                PathLine { x: 88.5; y: 252.2 }
                PathLine { x: 81.5; y: 264.3 }
            }
            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 74.7; startY: 264.3
                PathLine { x: 74.7; y: 264.3 }
                PathLine { x: 81.7; y: 276.5 }
                PathLine { x: 81.7; y: 252.2 }
                PathLine { x: 74.7; y: 264.3 }
            }

            TapHandler {
                onTapped: moveRelatively(2, 2)
            }
        }

        // Button A
        Shape {
            containsMode: Shape.FillContains

            ShapePath {
                strokeWidth: 1
                fillColor: "#444138"
                strokeColor: "#444138"
                startX: 0; startY: 0
                PathSvg { path: "M86.6,42.5V13.1c0-2.8-2.3-5.1-5.1-5.1H5.1C2.3,8,0,10.3,0,13.1v76.3c0,2.8,2.3,5.1,5.1,5.1h29.3
                                c1.6,0,3.1-0.7,4-2C52,75.5,67.5,60,84.6,46.5C85.8,45.5,86.6,44.1,86.6,42.5z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#666360"
                strokeColor: "#666360"
                startX: 0; startY: 0
                PathSvg { path: "M86.6,34.5V5.1c0-2.8-2.3-5.1-5.1-5.1H5.1C2.3,0,0,2.3,0,5.1v76.3c0,2.8,2.3,5.1,5.1,5.1h29.3
                                c1.6,0,3.1-0.7,4-2C52,67.5,67.5,52,84.6,38.5C85.8,37.5,86.6,36.1,86.6,34.5z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 0; startY: 0
                PathSvg { path: "M44.2,46.2h-8.7l-1.7,5.7h-7.2l9.6-29.3h3.6v0l0,0h3.6l9.6,29.3H46L44.2,46.2z M37.1,41h5.6l-2.7-9.1h-0.1
                    L37.1,41z" }
            }

            TapHandler {
                onTapped: moveToCorner(0)
            }
        }

        // Button B
        Shape {
            containsMode: Shape.FillContains

            ShapePath {
                strokeWidth: 1
                fillColor: "#444138"
                strokeColor: "#444138"
                startX: 0; startY: 0
                PathSvg { path: "M441.4,42.5V13.1c0-2.8,2.3-5.1,5.1-5.1h76.3c2.8,0,5.1,2.3,5.1,5.1v76.3c0,2.8-2.3,5.1-5.1,5.1h-29.3
                                    c-1.6,0-3.1-0.7-4-2c-13.5-17.1-29-32.6-46.1-46.1C442.2,45.5,441.4,44.1,441.4,42.5z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#666360"
                strokeColor: "#666360"
                startX: 0; startY: 0
                PathSvg { path: "M441.4,34.5V5.1c0-2.8,2.3-5.1,5.1-5.1l76.3,0c2.8,0,5.1,2.3,5.1,5.1v76.3c0,2.8-2.3,5.1-5.1,5.1h-29.3
                                    c-1.6,0-3.1-0.7-4-2c-13.5-17.1-29-32.6-46.1-46.1C442.2,37.5,441.4,36.1,441.4,34.5z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 0; startY: 0
                PathSvg { path: "M478.7,51.8V22.5h9.6c3.6,0,6.5,0.7,8.5,2c2,1.3,3,3.4,3,6c0,1.4-0.3,2.6-1,3.7s-1.6,1.9-3,2.4
                                c1.7,0.4,3,1.2,3.9,2.3c0.8,1.2,1.3,2.5,1.3,4.1c0,2.9-1,5-2.9,6.5c-1.9,1.5-4.7,2.2-8.2,2.2H478.7z M485.5,34.7h2.7
                                c1.6,0,2.9-0.3,3.7-0.9c0.8-0.6,1.2-1.4,1.2-2.5c0-1.2-0.4-2.2-1.2-2.7c-0.8-0.6-2-0.9-3.6-0.9h-2.8V34.7z M485.5,39.2v7.3h4.4
                                c1.4,0,2.5-0.3,3.3-0.9s1.1-1.4,1.1-2.6c0-1.2-0.3-2.2-1-2.8c-0.6-0.6-1.7-1-3.1-1h-0.2H485.5z" }
            }

            TapHandler {
                onTapped: moveToCorner(1)
            }
        }

        // Button C
        Shape {
            containsMode: Shape.FillContains

            ShapePath {
                strokeWidth: 1
                fillColor: "#444138"
                strokeColor: "#444138"
                startX: 0; startY: 0
                PathSvg { path: "M34.5,449.2H5.1c-2.8,0-5.1,2.3-5.1,5.1v76.3c0,2.8,2.3,5.1,5.1,5.1h76.3c2.8,0,5.1-2.3,5.1-5.1v-29.3
                        c0-1.6-0.7-3.1-2-4c-17.1-13.5-32.6-29-46.1-46.1C37.5,449.9,36.1,449.2,34.5,449.2z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#666360"
                strokeColor: "#666360"
                startX: 0; startY: 0
                PathSvg { path: "M34.5,441.2H5.1c-2.8,0-5.1,2.3-5.1,5.1v76.3c0,2.8,2.3,5.1,5.1,5.1h76.3c2.8,0,5.1-2.3,5.1-5.1v-29.3
                        c0-1.6-0.7-3.1-2-4c-17.1-13.5-32.6-29-46.1-46.1C37.5,441.9,36.1,441.2,34.5,441.2z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 0; startY: 0
                PathSvg { path: "M51.4,490l0,0.1c0.1,3.3-0.9,5.9-2.8,7.6c-1.9,1.7-4.7,2.6-8.2,2.6c-3.6,0-6.5-1.1-8.7-3.3
                    c-2.2-2.2-3.3-5.1-3.3-8.7v-6.1c0-3.6,1.1-6.5,3.2-8.7c2.1-2.2,4.9-3.3,8.4-3.3c3.7,0,6.6,0.9,8.6,2.6c2,1.7,3,4.2,3,7.5l-0.1,0.1
                    h-6.6c0-1.8-0.4-3.1-1.1-3.9c-0.8-0.8-2-1.2-3.8-1.2c-1.5,0-2.7,0.6-3.5,1.8c-0.8,1.2-1.2,2.9-1.2,4.9v6.2c0,2.1,0.4,3.7,1.3,4.9
                    c0.9,1.2,2.2,1.8,3.9,1.8c1.6,0,2.7-0.4,3.4-1.2c0.7-0.8,1-2.1,1-3.9H51.4z" }
            }

            TapHandler {
                onTapped: moveToCorner(2)
            }
        }

        // Button D
        Shape {
            containsMode: Shape.FillContains

            ShapePath {
                strokeWidth: 1
                fillColor: "#444138"
                strokeColor: "#444138"
                startX: 0; startY: 0
                PathSvg { path: "M493.2,449.2h29.3c2.8,0,5.1,2.3,5.1,5.1v76.3c0,2.8-2.3,5.1-5.1,5.1h-76.3c-2.8,0-5.1-2.3-5.1-5.1v-29.3
                        c0-1.6,0.7-3.1,2-4c17.1-13.5,32.6-29,46.1-46.1C490.2,449.9,491.7,449.2,493.2,449.2z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#666360"
                strokeColor: "#666360"
                startX: 0; startY: 0
                PathSvg { path: "M493.2,441.2h29.3c2.8,0,5.1,2.3,5.1,5.1v76.3c0,2.8-2.3,5.1-5.1,5.1h-76.3c-2.8,0-5.1-2.3-5.1-5.1v-29.3
                        c0-1.6,0.7-3.1,2-4c17.1-13.5,32.6-29,46.1-46.1C490.2,441.9,491.7,441.2,493.2,441.2z" }
            }

            ShapePath {
                strokeWidth: 1
                fillColor: "#FFFCF9"
                strokeColor: "#FFFCF9"
                startX: 0; startY: 0
                PathSvg { path: "M487.3,471.3c3.4,0,6.3,1.1,8.6,3.4c2.3,2.2,3.5,5.1,3.5,8.6v5.3c0,3.5-1.2,6.4-3.5,8.6
                    c-2.3,2.2-5.2,3.4-8.6,3.4h-10.7v-29.3H487.3z M487.3,495.3c1.5,0,2.8-0.6,3.8-1.9c1-1.3,1.5-2.9,1.5-4.8v-5.3
                    c0-2-0.5-3.6-1.5-4.9c-1-1.3-2.3-1.9-3.8-1.9h-3.9v18.8H487.3z" }
            }

            TapHandler {
                onTapped: moveToCorner(3)
            }
        }

        Rectangle {
            x: 0
            y: 553
            width: 260
            height: 50
            color: "transparent"

            Button {
                width: 260
                height: 60
                text: qsTr("Laser Pulse")
                font.pointSize: 28
                onClicked: laserPulse()
            }
        }

        Rectangle {
            x: 270
            y: 553
            width: 260
            height: 50
            color: "transparent"

            Button {
                width: 260
                height: 60
                checkable: true
                text: qsTr("Laser")
                font.pointSize: 28
                onClicked: laser()
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.75}
}
##^##*/
