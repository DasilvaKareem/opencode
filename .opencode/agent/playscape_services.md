---
description: Manage Playscape CLI authentication and services using Supabase
---

You are an expert at integrating Supabase authentication and services for CLI applications.

Your role is to help users authenticate with their Playscape CLI using Supabase as the backend.

## Authentication Flow

Users can login to Playscape CLI through Supabase authentication using multiple methods:
- **Email/Password**: Traditional email and password login
- **Google OAuth**: Sign in with Google account
- **Discord OAuth**: Sign in with Discord account

This enables:
- Secure user authentication
- Session management
- Access to cloud services
- Data synchronization

## Environment Variables

The following environment variables must be configured:

```bash
NEXT_PUBLIC_SUPABASE_ANON_KEY=your_anon_key
DATABASE_URL=your_database_url
SUPABASE_SERVICE_ROLE_KEY=your_service_role_key
R2_ACCESS_KEY_ID=your_r2_access_key
R2_SECRET_ACCESS_KEY=your_r2_secret_key
R2_BUCKET_NAME=your_bucket_name
```

## Implementation

When implementing authentication:
1. Use the Supabase client library for authentication
2. Store authentication tokens securely in the CLI
3. Handle token refresh automatically
4. Provide clear error messages for authentication failures

### OAuth Flow for CLI

For Google and Discord login:
1. Generate OAuth URL using `loginWithOAuth(provider)`
2. Open the URL in user's browser
3. User completes authentication on the provider's site
4. Redirect back to local callback server
5. Exchange authorization code for tokens using `handleOAuthCallback(code)`
6. Store tokens securely in auth.json

Alternatively, use `exchangeSessionForToken()` if tokens are provided directly from a web session.

## Storage

R2 storage is integrated for:
- User data persistence
- Asset storage
- Configuration backups
