<?php
if (!isset($_COOKIE["spelleider"]) || ($_COOKIE["spelleider"] != "spelleider")) {
    echo "Failed to set repairlevel: Unauthorized!";
}
$repairlevel = floatval($_POST['repairlevel']);
if ($repairlevel >= 0.0 && $repairlevel <= 1.0) {
    $res = file_put_contents('cmd/repair.txt', $repairlevel, LOCK_EX);
    if ((!$res) || ($res < 0)) {
        print_r(error_get_last());
    } else {
        echo "OK";
    }
} else {
    echo "Failed to set repairlevel ($repairlevel)";
}
?>
