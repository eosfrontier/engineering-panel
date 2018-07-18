var wid = 80;
var hei = wid * 0.866;
var side = 2;
var offX = 10;
var offY = 30;

var cellgrid = [];
var gametype = 0;

var typelist = [
    'Triangle',
    'Center double',
    'Hexes'
];

function click_type(ev) {
    if (ev.button != 0) return;
    gametype = gametype+1;
    if (gametype >= typelist.length) { gametype = 0; }
    reset_grid();
}

function reset_grid() {
    $('#gametype').text(typelist[gametype]);
    for (var row = 0; row < 2*side; row++) {
        for (var col = 0; col < (4*side)-1; col++) {
            if (cellgrid[row][col]) {
                cellgrid[row][col].color = 0;
            }
        }
    }
    draw_grid();
}

function draw_triangle(g, x, y, size, color) {
    g.beginPath();
    g.moveTo(x,y);
    g.lineTo(x-size*0.5,y+size*0.866);
    g.lineTo(x+size*0.5,y+size*0.866);
    g.closePath();
    g.fillStyle = color;
    g.fill();
    g.stroke();
}

var colors = [
    '#f00',
    '#00f',
    '#0f0'
];
function init_grid() {
    cellgrid[-1] = [];
    cellgrid[2*side] = [];
    for (var row = 0; row < 2*side; row++) {
        cellgrid[row] = [];
        var cut = (Math.abs(row-side+0.5)-0.5);
        for (var col = cut; col < (4*side)-1-cut; col++) {
            cellgrid[row][col] = {color: 0};
        }
    }
    $('#lock').on('click', click_grid);
    $('#gametype').on('click', click_type);
    reset_grid();
}

function click_grid(ev)
{
    var posX = ev.offsetX-offX;
    var posY = ev.offsetY-offY;
    var crdX, crdY
    if (gametype == 2) {
        crdY = Math.floor((posY+(hei/2))/hei);
        if (crdY % 2) {
            crdX = Math.floor(posX/wid) * 2;
        } else {
            crdX = Math.floor((posX+wid/2)/wid) * 2 - 1;
        }
        if (!cellgrid[crdY][crdX] && !cellgrid[crdY-1][crdX]) return;
    } else {
        /* Berekenen welke driehoek geklikt is */
        crdY = Math.floor(posY/hei);
        crdX = Math.floor(posX/wid);
        var insX = posX - crdX*wid - (wid/2);
        var insY = posY - crdY*hei;
        crdX *= 2;
        if (!(crdY % 2)) { insY = hei-insY; }
        if (insX < -(insY / 1.732)) { crdX -= 1;}
        if (insX >  (insY / 1.732)) { crdX += 1; }
        if (!cellgrid[crdY][crdX]) return;
    }

    /* Driehoek van kleur veranderen */
    var cells = [];
    if (gametype == 0) {
        cells.push(cellgrid[crdY][crdX], cellgrid[crdY][crdX-1], cellgrid[crdY][crdX+1]);
        if ((crdX+crdY) % 2) {
            cells.push(cellgrid[crdY+1][crdX]);
        } else {
            cells.push(cellgrid[crdY-1][crdX]);
        }
    }
    if (gametype == 1) {
        cells.push(cellgrid[crdY][crdX], cellgrid[crdY][crdX], cellgrid[crdY][crdX-1], cellgrid[crdY][crdX+1]);
        if ((crdX+crdY) % 2) {
            cells.push(cellgrid[crdY+1][crdX]);
        } else {
            cells.push(cellgrid[crdY-1][crdX]);
        }
    }
    if (gametype == 2) {
        cells.push(cellgrid[crdY][crdX-1]);
        cells.push(cellgrid[crdY][crdX]);
        cells.push(cellgrid[crdY][crdX+1]);
        cells.push(cellgrid[crdY-1][crdX-1]);
        cells.push(cellgrid[crdY-1][crdX]);
        cells.push(cellgrid[crdY-1][crdX+1]);
    }
    for (var i = 0; i < cells.length; i++) {
        if (cells[i]) {
            var col = cells[i].color;
            cells[i].color = (col+1) % colors.length;
        }
    }
    draw_grid();
}

function draw_grid() {
    var canvas = document.getElementById('lock');
    var g = canvas.getContext("2d");
    g.clearRect(0, 0, canvas.width, canvas.height);
    g.lineJoin = 'round';
    g.lineWidth = 8;
    g.strokeStyle = '#000';
    for (var row = 0; row < 2*side; row++) {
        var cut = (Math.abs(row-side+0.5)-0.5);
        for (var col = cut; col < (4*side)-1-cut; col++) {
            var ypos = row*hei;
            var size = -wid;
            if ((row+col+side) % 2) {
                size = wid;
            } else {
                ypos += hei;
            }
            draw_triangle(g, (col+1)*wid/2+offX, ypos+offY, size, colors[cellgrid[row][col].color]);
        }
    }
}

$(init_grid);
// vim: ai:si:expandtab:ts=4:sw=4
