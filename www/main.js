$(load)

function load()
{
    $.get('colors.txt', create_display)
    setInterval(reload, 2000)
}

/* Rij en pin, alweer tellend van rechtsonder */
function map_solution(r, p)
{
    if (r < 10) { p += 5 } else { r -= 10 }
    return (10*(9-r) + p)
}

/* Connector volgorde is rijtjes van 5, tellend vanaf rechtsonder, elk rijtje vlnr
 * vertalen naar rijtjes van 10, vanaf linksboven, vlnr
 */
function map_connector(c)
{
    var col = c % 5
    var row = (c-col)/5
    return map_solution(row, col)
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
    html.push('<tr id="switches" class="switches"><td colspan="5">')
    if ($('#connectors').attr('spelleider') == 'true') {
        html.push('<div class="button" myval="0.5">Break</div><div class="button" myval="1.0">Fix</div>')
    }
    html.push('</td>')
    for (var s = 0; s < 3; s++) {
        html.push('<td class="switch switch_',s,'"><div><div class="bar"></div></div></td>')
    }
    html.push('<td colspan="5">')
    html.push('<div id="repairlevel"><!--<div class="display"></div>--><div class="full"></div></div></td></tr>')
    html.push('<tr class="connectors">')
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
    $('#switches div.button').click(set_repair)
    reload()
}

function set_repair()
{
    var repairlevel = $(this).attr('myval')
    $(this).addClass('clicked')
    $.post('set_repairlevel.php', { repairlevel: repairlevel }, clicked_repair)
}

function clicked_repair(txt)
{
    $('#switches div.button').removeClass('clicked')
}

function reload()
{
    $.ajax({
        type:'GET',
        url:'connections.json',
        success:display_connections,
        ifModified:true,
        datatype:'json'
    })
}

var lastts = 0

function display_connections(json)
{
    if (!json) { return }
    if (lastts == json.timestamp) { return }
    lastts = json.timestamp
    for (var s = 0; s < json.switches.length; s++) {
        if (json.switches[s] & 0x200) {
            $('#switches .switch_'+s).addClass('on')
        } else {
            $('#switches .switch_'+s).removeClass('on')
        }
    }
    for (var t = 0; t < json.turbines.length; t++) {
        $('#switches .switch_'+t+' div.bar').height(100*json.turbines[t]+'%')
    }
    $('#repairlevel .full').width((100*(json.repairlevel))+'%')
    $('#repairlevel .display').text(Math.round(100*(json.repairlevel))+'%')
    var connectors = $('#connectors td.connector').removeClass('connected current solution').get()
    for (var c = 0; c < json.connections.length; c++) {
        var conn = json.connections[c]
        $(connectors[map_connector(conn[0])]).addClass('connected')
        $(connectors[map_connector(conn[1])]).addClass('connected')
    }
    for (var r = 0; r < json.rows.length; r++) {
        var cur = json.rows[r][0]
        var sol = json.rows[r][1]
        for (var p = 0; p < 5; p++) {
            if (cur & (1 << p)) {
                $(connectors[map_solution(r, p)]).addClass('current')
            }
            if (sol & (1 << p)) {
                $(connectors[map_solution(r, p)]).addClass('solution')
            }
        }
    }
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
