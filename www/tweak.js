$(load)
reload()

function load()
{
    $('#settings').on('change', 'td.value input', set_setting)
}

function set_setting()
{
    $.post('set_setting.php', { key : $(this).attr('mykey'), value: $(this).val() }, update_settings)
}

function reload()
{
    $.get('settings.json', show_settings)
}

function show_settings(json)
{
    for (var s = 0; s < json.settings.length; s++) {
        var st = json.settings[s]
        $('#settings').append($('<tr class="'+st.key+'"><td class="key">'+
            st.description+'</td>'+'<td class="value">'+
            '<input type="text" mykey="'+st.key+'" value="'+st.value+'"></td></tr>'))
    }
}

function update_settings(json)
{
    if (json.settings) {
        for (s in json.settings) {
            $('#settings tr.'+s+' .value input').val(json.settings[s])
        }
    }
    if (json.error) { alert(json.error) }
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
