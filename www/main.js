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
    html.push('<div id="difficulty"><div>Difficulty</div>')
    if ($('#connectors').attr('spelleider') == 'true') {
        html.push('<span class="button lower" myval="-1">&lt;</span><span class="value"></span><span class="button higher" myval="+1">&gt;</span>')
    } else {
        html.push('<span class="value"></span>')
    }
    html.push('</div></td>')
    for (var s = 0; s < 3; s++) {
        html.push('<td class="switch switch_',s,'"><div><div class="bar"></div></div></td>')
    }
    html.push('<td colspan="5">')
    html.push('<div>Power</div>')
    html.push('<div id="repairlevel"><div class="full"></div></div></td></tr>')
    html.push('<tr class="connectors">')
    var colorlist = [ 'black', 'blue', 'green', 'yellow', 'red' ]
    for (var c = 0; c < 100; c++) {
        if ((c % 20) == 5) {
            html.push('<td class="center ',colorlist[(c-5)/20],'" rowspan="2" colspan="3"></td>')
        }
        if (c > 0 && (c % 10 == 0)) html.push('</tr><tr class="connectors">')
        html.push('<td class="connector" myrow="',Math.floor(c/5),'"><div></div></td>')
    }
    html.push('</tr>')
    $('#connectors').html(html.join(''))
    var connectors = $('#connectors td.connector').get()
    for (var c = 0; c < colors.length; c++) {
        $(connectors[map_connector(c)]).addClass(colormap[colors[c]])
    }
    $('table.break .button').click(set_repair)
    $('#difficulty .button').click(set_difficulty)
    $.get('set_setting.php', show_settings)
    reload()
}

function set_difficulty()
{
    if (!$(this).hasClass('disabled')) {
        var d = $(this).attr('myval')
        if (d) {
            $.post('set_setting.php', { key: 'difficulty', value: d }, show_settings)
        }
    }
}

function show_settings(json)
{
    if (json.settings) {
        var d = json.settings.difficulty
        $('#difficulty .value').text(d)
        if (d > 1) {
            $('#difficulty .lower').removeClass('disabled').attr('myval', d-1)
        } else {
            $('#difficulty .lower').addClass('disabled').attr('myval', d)
        }
        if (d < 2) {
            $('#difficulty .higher').removeClass('disabled').attr('myval', d+1)
        } else {
            $('#difficulty .higher').addClass('disabled').attr('myval', d)
        }
    }
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
    var connectors = $('#connectors td.connector').removeClass('connected current solution b_low b_good b_high').get()
    for (var c = 0; c < json.connections.length; c++) {
        var conn = json.connections[c]
        $(connectors[map_connector(conn[0])]).addClass('connected')
        $(connectors[map_connector(conn[1])]).addClass('connected')
    }
    if (json.solution) {
        for (var r = 0; r < json.solution.length; r++) {
            $(connectors[map_connector(json.solution[r])]).addClass('solution')
        }
    }
    if (json.balance) {
        for (var c in json.balance) {
            var cls = 'b_good';
            if (json.balance[c][0] < json.balance[c][1]) cls = 'b_low'
            if (json.balance[c][0] > json.balance[c][1]) cls = 'b_high'
            $('#connectors td.connector.'+c).addClass(cls)
        }
        $('#connectors td.connector.connected.b_good,#connectors td.connector.connected.b_low').each(function() {
            var mr = $(this).attr('myrow')
            $('#connectors td.connector[myrow='+mr+'].b_low:not(.connected)').removeClass('b_low')
        });
    }
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
