$(load)
setInterval(reload, 10000)
reload()

function load()
{
    $('#VolDown,#VolUp').click(set_volume)
    $('#settings').on('change', 'td.value input', set_setting)
}

function set_volume()
{
    var curvol = parseInt($('#Volume').text()) + parseInt($(this).attr('volchange'));
    if (curvol < 0) curvol = 0
    if (curvol > 100) curvol = 100
    $.post('set_volume.php', { volume: curvol }, show_info)
}

function set_setting()
{
    $.post('set_setting.php', { key : $(this).attr('mykey'), value: $(this).val() }, show_settings)
}

function reload()
{
    $.get('sysinfo.php', show_info)
    $.get('set_setting.php', show_settings)
}

function show_settings(json)
{
    if (json.settings) {
        for (s in json.settings) {
            if (!$('#settings tr.'+s).length) {
                $('#settings').append($('<tr class="'+s+'"><td class="key">'+s+'</td>'+
                    '<td class="value"><input type="text" mykey="'+s+'"></td></tr>'))
            }
            $('#settings tr.'+s+' .value input').val(json.settings[s])
        }
    }
    if (json.error) { alert(json.error) }
}

function show_info(json)
{
    for (inf in json.sysinfo) {
        $('#'+inf).text(json.sysinfo[inf])
    }
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
