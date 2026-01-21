# Operational Playbook

This document provides step-by-step procedures for deploying, operating, and maintaining the Core SaaS Platform.

## Table of Contents

1. [Initial Deployment](#initial-deployment)
2. [Starting the System](#starting-the-system)
3. [Stopping the System](#stopping-the-system)
4. [Backup Procedures](#backup-procedures)
5. [Restore Procedures](#restore-procedures)
6. [Monitoring](#monitoring)
7. [Log Management](#log-management)
8. [Database Maintenance](#database-maintenance)
9. [Billing Operations](#billing-operations)
10. [Troubleshooting](#troubleshooting)

---

## Initial Deployment

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential libsqlite3-dev sqlite3

# CentOS/RHEL
sudo yum groupinstall "Development Tools"
sudo yum install -y sqlite-devel sqlite

# macOS
brew install sqlite3
```

### Build the Application

```bash
# Clone or copy the source code
cd /opt/cero

# Check dependencies
make check-deps

# Build the binary
make

# The binary is now available as ./cero
```

### Initial Configuration

```bash
# Create secrets file from template
cp config/secrets.txt.template config/secrets.txt

# Generate session secret
openssl rand -hex 32

# Generate CSRF secret
openssl rand -hex 32

# Edit secrets file and add the generated values
nano config/secrets.txt

# Generate admin password hash (cost factor 12)
# Install htpasswd: apt-get install apache2-utils
htpasswd -bnBC 12 "" your_admin_password | tr -d ':\n'

# Add the hash to secrets.txt
nano config/secrets.txt
```

### Initialize Database

```bash
# Initialize database with schema
make init-db

# Verify database was created
sqlite3 data/app.db "SELECT version, description FROM schema_version;"
```

### Create First Admin User (Optional)

```bash
# Connect to database
sqlite3 data/app.db

# Create account
INSERT INTO accounts (id, name, created_at, status)
VALUES (1, 'System Admin', strftime('%s', 'now'), 'active');

# Create admin user (update email and password hash)
INSERT INTO users (account_id, email, password_hash, role, created_at)
VALUES (1, 'admin@example.com', '$2b$12$YOUR_HASH_HERE', 'admin', strftime('%s', 'now'));

# Create free subscription
INSERT INTO subscriptions (account_id, plan, status, valid_from, valid_until, created_at, updated_at)
VALUES (1, 'free', 'active', strftime('%s', 'now'), strftime('%s', 'now', '+100 years'), strftime('%s', 'now'), strftime('%s', 'now'));

.quit
```

### Configure Reverse Proxy (Recommended)

Use nginx for TLS termination and static file serving:

```nginx
# /etc/nginx/sites-available/cero
server {
    listen 80;
    server_name example.com;

    # Redirect to HTTPS
    return 301 https://$host$request_uri;
}

server {
    listen 443 ssl http2;
    server_name example.com;

    ssl_certificate /etc/letsencrypt/live/example.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/example.com/privkey.pem;

    # Security headers
    add_header Strict-Transport-Security "max-age=31536000" always;
    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-Content-Type-Options "nosniff" always;

    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

---

## Starting the System

### Manual Start

```bash
cd /opt/cero
./cero

# With custom config
./cero config/config.txt config/secrets.txt config/schema.sql
```

### systemd Service (Recommended)

Create `/etc/systemd/system/cero.service`:

```ini
[Unit]
Description=Core SaaS Platform
After=network.target

[Service]
Type=simple
User=cero
Group=cero
WorkingDirectory=/opt/cero
ExecStart=/opt/cero/cero
Restart=always
RestartSec=10

# Security
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/opt/cero/data /opt/cero/logs /opt/cero/backups

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable cero
sudo systemctl start cero
sudo systemctl status cero
```

---

## Stopping the System

### Graceful Shutdown

```bash
# If running manually
# Press Ctrl+C

# If running as systemd service
sudo systemctl stop cero

# Check status
sudo systemctl status cero
```

---

## Backup Procedures

### Daily Backup (Automated)

Add to crontab:

```bash
# Edit crontab
crontab -e

# Add daily backup at 2 AM
0 2 * * * /opt/cero/scripts/backup.sh
```

Create `/opt/cero/scripts/backup.sh`:

```bash
#!/bin/bash
set -e

BACKUP_DIR="/opt/cero/backups"
DATE=$(date +%Y-%m-%d)

# Backup database
cp /opt/cero/data/app.db "$BACKUP_DIR/app.db.$DATE"

# Backup config and secrets
tar -czf "$BACKUP_DIR/config.$DATE.tar.gz" /opt/cero/config/

# Keep only last 30 days
find "$BACKUP_DIR" -name "app.db.*" -mtime +30 -delete
find "$BACKUP_DIR" -name "config.*.tar.gz" -mtime +30 -delete

# Log backup
echo "$(date): Backup completed successfully" >> /opt/cero/logs/backup.log
```

Make executable:

```bash
chmod +x /opt/cero/scripts/backup.sh
```

### Manual Backup

```bash
# Using make
make backup

# Or manually
cp data/app.db backups/app.db.$(date +%Y-%m-%d-%H%M%S)
```

### Off-site Backup

```bash
# Using rsync to remote server
rsync -avz --delete \
  /opt/cero/backups/ \
  backup-server:/backups/cero/

# Using cloud storage (example with rclone)
rclone sync /opt/cero/backups/ remote:cero-backups/
```

---

## Restore Procedures

### Full System Restore

```bash
# 1. Stop the application
sudo systemctl stop cero

# 2. Restore database
cp backups/app.db.2026-01-21 data/app.db

# 3. Restore config (if needed)
tar -xzf backups/config.2026-01-21.tar.gz -C /

# 4. Verify database integrity
sqlite3 data/app.db "PRAGMA integrity_check;"

# 5. Start application
sudo systemctl start cero

# 6. Verify
sudo systemctl status cero
curl -I http://localhost:8080/
```

### Partial Data Recovery

```bash
# If only certain data is corrupted, can selectively restore

# 1. Dump specific tables from backup
sqlite3 backups/app.db.2026-01-21 ".dump users" > users_backup.sql

# 2. Stop application
sudo systemctl stop cero

# 3. Restore specific data
sqlite3 data/app.db < users_backup.sql

# 4. Start application
sudo systemctl start cero
```

---

## Monitoring

### Health Check

Create `/opt/cero/scripts/health-check.sh`:

```bash
#!/bin/bash

# Check if process is running
if ! pgrep -x "cero" > /dev/null; then
    echo "ERROR: cero process not running"
    exit 1
fi

# Check if responding to HTTP
if ! curl -sf http://localhost:8080/ > /dev/null; then
    echo "ERROR: cero not responding to HTTP"
    exit 1
fi

# Check database file exists and is readable
if [ ! -r /opt/cero/data/app.db ]; then
    echo "ERROR: database file not readable"
    exit 1
fi

# Check disk space
DISK_USAGE=$(df /opt/cero | tail -1 | awk '{print $5}' | sed 's/%//')
if [ "$DISK_USAGE" -gt 90 ]; then
    echo "WARNING: disk usage at ${DISK_USAGE}%"
fi

echo "OK: All checks passed"
exit 0
```

Run via cron:

```bash
# Check every 5 minutes
*/5 * * * * /opt/cero/scripts/health-check.sh || mail -s "cero health check failed" admin@example.com
```

### Manual Monitoring

```bash
# Check if running
systemctl status cero

# Check logs
tail -f logs/app.log

# Check database size
du -h data/app.db

# Check active sessions
sqlite3 data/app.db "SELECT COUNT(*) FROM sessions WHERE expires_at > strftime('%s', 'now');"

# Check disk usage
df -h /opt/cero
```

---

## Log Management

### Log Rotation

Application rotates logs internally when `log_rotate()` is called.

Manual rotation:

```bash
# Rotate current log
mv logs/app.log logs/app.log.$(date +%Y-%m-%d)

# Application will create new log file automatically
```

Automated rotation via cron:

```bash
# Add to crontab - rotate daily at midnight
0 0 * * * /opt/cero/scripts/rotate-logs.sh
```

Create `/opt/cero/scripts/rotate-logs.sh`:

```bash
#!/bin/bash
DATE=$(date +%Y-%m-%d)

# Rotate application log
if [ -f /opt/cero/logs/app.log ]; then
    mv /opt/cero/logs/app.log "/opt/cero/logs/app.log.$DATE"
fi

# Compress old logs (older than 7 days)
find /opt/cero/logs -name "app.log.*" -mtime +7 -exec gzip {} \;

# Delete very old logs (older than 90 days)
find /opt/cero/logs -name "app.log.*.gz" -mtime +90 -delete
```

---

## Database Maintenance

### Vacuum Database (Monthly)

Reclaim space and optimize:

```bash
# Stop application
sudo systemctl stop cero

# Vacuum database
sqlite3 data/app.db "VACUUM;"

# Analyze for query optimization
sqlite3 data/app.db "ANALYZE;"

# Start application
sudo systemctl start cero
```

### Clean Up Old Data

```bash
# Delete expired sessions (safe to run while app is running)
sqlite3 data/app.db "DELETE FROM sessions WHERE expires_at < strftime('%s', 'now');"

# Delete old rate limit entries
sqlite3 data/app.db "DELETE FROM rate_limits WHERE last_refill_at < strftime('%s', 'now', '-7 days');"

# Delete expired report cache
sqlite3 data/app.db "DELETE FROM report_cache WHERE expires_at < strftime('%s', 'now');"
```

### Database Integrity Check

```bash
# Run integrity check
sqlite3 data/app.db "PRAGMA integrity_check;"

# Should output: ok
```

---

## Billing Operations

### Mark Customer as Paid (Manual Workflow)

1. **Receive Payment Confirmation**
   - Customer emails receipt/confirmation
   - Verify payment in bank account or payment processor

2. **Login to Admin Interface**
   ```
   Navigate to: https://example.com/admin/billing
   Login with admin credentials
   ```

3. **Search for Customer**
   - Enter customer email or account name
   - Click "Search"

4. **Update Subscription**
   - Click "Manage" next to customer account
   - Fill in payment details:
     - Plan: pro
     - Duration: 30 days (or 365 for yearly)
     - Amount: 4900 cents ($49.00)
     - Payment method: bank_transfer (or appropriate method)
     - External reference: Invoice number, receipt ID, etc.
     - Notes: Any relevant information
   - Click "Mark as Paid & Update Subscription"

5. **Verify**
   - Customer should immediately see Pro features enabled
   - Billing event logged in billing_events table
   - Audit entry created in audit_log

### Manual Database Billing Update (If Needed)

```bash
sqlite3 data/app.db

-- Update subscription
UPDATE subscriptions
SET plan = 'pro',
    status = 'active',
    valid_until = strftime('%s', 'now', '+30 days'),
    updated_at = strftime('%s', 'now')
WHERE account_id = 123;

-- Log billing event
INSERT INTO billing_events (
    account_id, event_type, previous_plan, new_plan,
    previous_status, new_status, amount_cents, currency,
    payment_method, external_reference, admin_user_id,
    notes, occurred_at
) VALUES (
    123, 'payment_received', 'free', 'pro',
    'active', 'active', 4900, 'USD',
    'bank_transfer', 'REF-12345', 1,
    'Manual update by admin', strftime('%s', 'now')
);

.quit
```

---

## Troubleshooting

### Application Won't Start

```bash
# Check logs
tail -f logs/app.log

# Check if port is already in use
sudo netstat -tulpn | grep 8080

# Check database file permissions
ls -l data/app.db

# Check config file syntax
cat config/config.txt

# Try running manually with debug
./cero
```

### Database Locked Errors

```bash
# Check for other processes accessing database
lsof data/app.db

# Ensure WAL mode is enabled
sqlite3 data/app.db "PRAGMA journal_mode;"
# Should output: wal

# If not, enable it
sqlite3 data/app.db "PRAGMA journal_mode=WAL;"
```

### High Memory Usage

```bash
# Check process memory
ps aux | grep cero

# Check database size
du -h data/app.db

# Consider vacuuming if database is large
sudo systemctl stop cero
sqlite3 data/app.db "VACUUM;"
sudo systemctl start cero
```

### Slow Queries

```bash
# Enable query logging (if implemented)
# Or use SQLite's explain query plan

sqlite3 data/app.db
EXPLAIN QUERY PLAN SELECT * FROM users WHERE email = 'test@example.com';
```

### Cannot Login

```bash
# Verify user exists
sqlite3 data/app.db "SELECT * FROM users WHERE email = 'user@example.com';"

# Reset password (generate new hash first)
sqlite3 data/app.db
UPDATE users
SET password_hash = '$2b$12$NEW_HASH_HERE'
WHERE email = 'user@example.com';
.quit
```

---

## Emergency Procedures

### Complete System Failure

1. **Stop application**
   ```bash
   sudo systemctl stop cero
   ```

2. **Restore from last known good backup**
   ```bash
   cp backups/app.db.LAST_GOOD_DATE data/app.db
   ```

3. **Verify database integrity**
   ```bash
   sqlite3 data/app.db "PRAGMA integrity_check;"
   ```

4. **Restart application**
   ```bash
   sudo systemctl start cero
   ```

5. **Monitor logs**
   ```bash
   tail -f logs/app.log
   ```

### Data Corruption

If database is corrupted but recent backup is not available:

```bash
# Attempt to dump as much data as possible
sqlite3 data/app.db .dump > dump.sql

# Create new database
mv data/app.db data/app.db.corrupt
sqlite3 data/app.db < config/schema.sql

# Import dumped data
sqlite3 data/app.db < dump.sql

# If errors occur, manually reconstruct critical tables
```

---

## Operational Checklist

### Daily
- [ ] Check application is running
- [ ] Review error logs
- [ ] Verify automated backup completed

### Weekly
- [ ] Review disk space usage
- [ ] Test database backup restoration
- [ ] Review billing events
- [ ] Clean up expired sessions

### Monthly
- [ ] Vacuum database
- [ ] Review and rotate logs
- [ ] Update TLS certificates (if needed)
- [ ] Review audit log for anomalies

### Quarterly
- [ ] Test complete disaster recovery procedure
- [ ] Review security settings
- [ ] Update documentation for any changes
- [ ] Review and optimize database indexes

### Annually
- [ ] Review and update admin credentials
- [ ] Archive old logs and backups
- [ ] Review long-term storage usage trends
- [ ] Consider OS and dependency updates

---

## Support Contacts

For issues beyond this playbook:

- System administrator: admin@example.com
- Database issues: Check SQLite documentation
- Application code: Review source code in /opt/cero/src/

---

**Remember**: This system is designed to be boring and reliable. If something seems complex or magical, it's probably a mistake. Keep operations simple and manual.
