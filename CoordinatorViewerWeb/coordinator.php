<?php

include 'env.php';

// Get the full query string
$fullQuery = $_SERVER['QUERY_STRING'];

// Find the position of 'query='
$pos = strpos($fullQuery, 'query=');
if ($pos === false) {
    http_response_code(400);
    echo "Missing 'query' parameter.";
    exit;
}

// Extract everything after 'query='
$rawQuery = substr($fullQuery, $pos + 6); // 6 = length of 'query='

// Build the target URL
$targetUrl = "http://192.168.2.216" . $rawQuery;

// Initialize cURL session
$ch = curl_init();
curl_setopt($ch, CURLOPT_URL, $targetUrl);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_USERPWD, "$username:$password");
curl_setopt($ch, CURLOPT_HEADER, true);

$response = curl_exec($ch);

if (curl_errno($ch)) {
    http_response_code(500);
    echo "cURL error: " . curl_error($ch);
    curl_close($ch);
    exit;
}

$headerSize = curl_getinfo($ch, CURLINFO_HEADER_SIZE);
$headers = substr($response, 0, $headerSize);
$body = substr($response, $headerSize);

// Forward the original Content-Type
if (preg_match('/Content-Type:\s*([^\r\n]+)/i', $headers, $matches)) {
    header("Content-Type: " . trim($matches[1]));
} else {
    header("Content-Type: application/octet-stream");
}

echo $body;
curl_close($ch);
?>
