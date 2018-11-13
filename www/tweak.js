$(load)
reload()

function load()
{
    $('#settings').on('change', 'td.value input', set_setting)
}

function set_setting()
{
    $.post('set_setting.php', { key : $(this).attr('mykey'), value: $(this).val() }, show_settings)
}

function reload()
{
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

/* vim: ai:si:expandtab:ts=4:sw=4
 */
