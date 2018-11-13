<?php
header('Content-Type: application/json');
$key = $_POST['key'];
$result = array();
if ($key) {
    if (!isset($_COOKIE["spelleider"]) || ($_COOKIE["spelleider"] != "spelleider")) {
        $result["error"] = "Failed to set setting: Unauthorized!";
    } else {
        if (!file_exists("settings/$key")) {
            $result["error"] = "Unknown setting $key";
        } else {
            $value = $_POST['value'];
            if (preg_match('/^[0-9.]+$/', $value)) {
                $value = floatval($value);
                $res = file_put_contents("settings/$key", $value, LOCK_EX);
                if ((!$res) || ($res < 0)) {
                    $result["error"] = (error_get_last());
                } else {
                    $result["result"] = "OK";
                }
            } else {
                $result["error"] = "Failed to parse setting ($key = $value)";
            }
        }
    }
}
$result["settings"] = array();
foreach(scandir('settings/') as $setting) {
    if ($setting[0] != '.') {
        $result["settings"][$setting] = floatval(file_get_contents("settings/$setting"));
    }
}
echo json_encode($result);
?>
