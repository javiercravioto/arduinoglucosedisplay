<?php
error_reporting(E_ALL);
ini_set('display_errors', 1);


// LibreLinkUp credentials
$email = 'test@gmail.com'; // librelinkup account email
$password = 'password'; //librelinkup account password


// Config

$baseUrl = 'https://api-la.libreview.io';

// Function to send cURL requests
function sendRequest($url, $method = 'GET', $headers = [], $postData = null)
{
    $ch = curl_init($url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    if ($method === 'POST') {
        curl_setopt($ch, CURLOPT_POST, true);
        if ($postData) {
            curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($postData));
        }
    }
    if (!empty($headers)) {
        curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
    }
    $response = curl_exec($ch);
    $httpcode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);
    return [$response, $httpcode];
}

// Step 1: Login
$loginUrl = "$baseUrl/llu/auth/login";
$headers = [
    'Content-Type: application/json',
    'product: llu.android',
    'version: 4.10',
    'User-Agent: okhttp/3.12.1'
];
$loginPayload = [
    'email' => $email,
    'password' => $password
];

list($loginResponse, $loginCode) = sendRequest($loginUrl, 'POST', $headers, $loginPayload);
$loginJson = json_decode($loginResponse, true);

if ($loginCode !== 200 || empty($loginJson['data']['authTicket']['token'])) {
    echo json_encode(['error' => 'Login failed', 'response' => $loginResponse]);
    exit;
}

$token = $loginJson['data']['authTicket']['token'];

// Step 2: Get Connections
$connUrl = "$baseUrl/llu/connections";
$headers[] = "Authorization: Bearer $token";
list($connResponse, $connCode) = sendRequest($connUrl, 'GET', $headers);
$connJson = json_decode($connResponse, true);

if ($connCode !== 200 || empty($connJson['data'][0]['patientId'])) {
    echo json_encode(['error' => 'Failed to fetch connection', 'response' => $connResponse]);
    exit;
}

$patientId = $connJson['data'][0]['patientId'];

// Step 3: Get Glucose Data
$glucoseUrl = "$baseUrl/llu/connections/$patientId/graph";
list($glucoseResponse, $glucoseCode) = sendRequest($glucoseUrl, 'GET', $headers);
$glucoseJson = json_decode($glucoseResponse, true);

if ($glucoseCode !== 200 || empty($glucoseJson['data']['graphData'])) {
    echo json_encode(['error' => 'Failed to fetch glucose data', 'response' => $glucoseResponse]);
    exit;
}

// Get the latest reading
$latest = $glucoseJson['data']['graphData'][0]['Value'] ?? null;
if ($latest === null) {
    echo json_encode(['error' => 'No glucose value found']);
    exit;
}

echo json_encode(['glucose' => $latest]);

?>
