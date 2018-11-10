setInterval(reload, 10000)
reload()

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
