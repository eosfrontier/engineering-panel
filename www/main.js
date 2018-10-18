$(load)

function load()
{
    $.get('colors.txt', create_display)
    setInterval(reload, 1000)
}

/* Connector volgorde is rijtjes van 5, tellend vanaf rechtsonder, elk rijtje vlnr
 * vertalen naar rijtjes van 10, vanaf linksboven, vlnr
 */
function map_connector(c)
{
    var col = c % 5
    var row = (c-col)/5
    if (row < 10) { col += 5 } else { row -= 10 }
    return (10*(9-row) + col)
}

function create_display(colors)
{
    var colormap = {
        'R':'red',
        'G':'green',
        'B':'blue',
        'Z':'black',
        'Y':'yellow'
    }
    var html = []
    html.push('<tr id="switches" class="switches"><td colspan="5"></td>')
    for (var s = 0; s < 3; s++) {
        html.push('<td class="switch switch_',s,'"><div></div></td>');
    }
    html.push('<td colspan="5"></td></tr><tr class="connectors">')
    for (var c = 0; c < 100; c++) {
        if (c % 10 == 5) html.push('<td class="center" colspan="3"></td>')
        if (c > 0 && (c % 10 == 0)) html.push('</tr><tr class="connectors">')
        html.push('<td class="connector"><div></div></td>')
    }
    html.push('</tr>')
    $('#connectors').html(html.join(''))
    var connectors = $('#connectors td.connector').get()
    for (var c = 0; c < colors.length; c++) {
        $(connectors[map_connector(c)]).addClass(colormap[colors[c]])
    }
    reload()
}

function reload()
{
    $.get('connections.json', display_connections)
}

function display_connections(json)
{
    for (var s = 0; s < json.switches.length; s++) {
        if (json.switches[s] & 0x200) {
            $('#switches .switch_'+s).addClass('on')
        } else {
            $('#switches .switch_'+s).removeClass('on')
        }
    }
    var connectors = $('#connectors td.connector').removeClass('connected').get()
    for (var c = 0; c < json.connections.length; c++) {
        var conn = json.connections[c]
        $(connectors[map_connector(conn[0])]).addClass('connected')
        $(connectors[map_connector(conn[1])]).addClass('connected')
    }
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
