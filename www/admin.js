$(load)
setInterval(reload, 10000)
reload()

function load()
{
    $('#VolDown,#VolUp').click(set_volume)
}

function set_volume()
{
    var curvol = parseInt($('#Volume').text()) + parseInt($(this).attr('volchange'));
    if (curvol < 0) curvol = 0
    if (curvol > 100) curvol = 100
    $.post('set_volume.php', { volume: curvol }, show_info)
}

function reload()
{
    $.get('sysinfo.php', show_info)
}

function show_info(json)
{
    for (inf in json.sysinfo) {
        $('#'+inf).text(json.sysinfo[inf])
    }
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
