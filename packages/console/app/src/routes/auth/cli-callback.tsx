import { createAsync, useSearchParams } from "@solidjs/router"
import type { APIEvent } from "@solidjs/start/server"

export async function GET(input: APIEvent) {
  // Return HTML that extracts tokens from URL fragment
  return new Response(
    `<!DOCTYPE html>
<html>
<head>
  <title>CLI Authentication</title>
  <style>
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      margin: 0;
      background: #0a0a0a;
      color: #fff;
    }
    .container {
      text-align: center;
      padding: 2rem;
      max-width: 600px;
    }
    .code-box {
      background: #1a1a1a;
      border: 2px solid #333;
      border-radius: 8px;
      padding: 1.5rem;
      margin: 2rem 0;
      word-break: break-all;
      font-family: 'Monaco', 'Courier New', monospace;
      font-size: 14px;
      position: relative;
    }
    .copy-btn {
      background: #0066ff;
      color: white;
      border: none;
      padding: 0.75rem 1.5rem;
      border-radius: 6px;
      font-size: 16px;
      cursor: pointer;
      margin-top: 1rem;
      transition: background 0.2s;
    }
    .copy-btn:hover {
      background: #0052cc;
    }
    .copy-btn:active {
      background: #003d99;
    }
    .success {
      color: #00ff00;
      margin-top: 1rem;
      opacity: 0;
      transition: opacity 0.3s;
    }
    .success.show {
      opacity: 1;
    }
    .error {
      background: #ff4444;
      padding: 1rem;
      border-radius: 8px;
      margin-top: 1rem;
    }
    h1 {
      margin-bottom: 0.5rem;
    }
    p {
      color: #999;
      line-height: 1.6;
    }
  </style>
</head>
<body>
  <div class="container" id="container">
    <h1>🔄 Processing...</h1>
    <p>Extracting authentication tokens...</p>
  </div>

  <script>
    // Extract tokens from URL fragment
    const hash = window.location.hash.substring(1);
    const params = new URLSearchParams(hash);
    const accessToken = params.get('access_token');
    const refreshToken = params.get('refresh_token');

    const container = document.getElementById('container');

    if (accessToken && refreshToken) {
      container.innerHTML = \`
        <h1>✅ Authentication Successful</h1>
        <p>Copy both tokens below and paste them into your terminal when prompted:</p>

        <h3 style="margin-top: 2rem; color: #999;">Access Token:</h3>
        <div class="code-box" id="access-token">\${accessToken}</div>
        <button class="copy-btn" onclick="copyToken('access-token', 'access-success')">📋 Copy Access Token</button>
        <div class="success" id="access-success">Copied!</div>

        <h3 style="margin-top: 2rem; color: #999;">Refresh Token:</h3>
        <div class="code-box" id="refresh-token">\${refreshToken}</div>
        <button class="copy-btn" onclick="copyToken('refresh-token', 'refresh-success')">📋 Copy Refresh Token</button>
        <div class="success" id="refresh-success">Copied!</div>
      \`;
    } else {
      container.innerHTML = \`
        <h1>❌ Error</h1>
        <div class="error">
          <p>No authentication tokens found in the URL.</p>
        </div>
      \`;
    }

    function copyToken(tokenId, successId) {
      const token = document.getElementById(tokenId).textContent;
      navigator.clipboard.writeText(token).then(() => {
        document.getElementById(successId).classList.add('show');
        setTimeout(() => {
          document.getElementById(successId).classList.remove('show');
        }, 3000);
      });
    }
  </script>
</body>
</html>`,
    {
      headers: { "Content-Type": "text/html" },
    }
  )
}

export default function CLICallback() {
  const [searchParams] = useSearchParams()
  const code = createAsync(() => Promise.resolve(searchParams.code))

  return (
    <div>
      <h1>CLI Authentication</h1>
      {code() ? (
        <div>
          <p>Copy this code and paste it into your terminal:</p>
          <code>{code()}</code>
        </div>
      ) : (
        <p>No authorization code found.</p>
      )}
    </div>
  )
}
