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

var prevcpu = {
    user: 0,
    nice: 0,
    system: 0,
    idle: 0
}

function show_info(json)
{
    for (inf in json.sysinfo) {
        $('#'+inf).text(json.sysinfo[inf])
    }
    if (json.sysinfo.CpuUser) {
        var user = json.sysinfo.CpuUser - prevcpu.user
        var nice = json.sysinfo.CpuNice - prevcpu.nice
        var system = json.sysinfo.CpuSystem - prevcpu.system
        var idle = json.sysinfo.CpuIdle - prevcpu.idle
        prevcpu.user = json.sysinfo.CpuUser
        prevcpu.nice = json.sysinfo.CpuNice
        prevcpu.system = json.sysinfo.CpuSystem
        prevcpu.idle = json.sysinfo.CpuIdle
        $('#CpuUsage').text((100*(user+nice+system)/(user+nice+system+idle)).toFixed(1))
    }
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
