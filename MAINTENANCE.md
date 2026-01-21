# Long-Term Maintenance Guide

## Philosophy

This system is designed to require minimal maintenance over a 50+ year lifespan. This document outlines the **only** maintenance tasks that should ever be necessary.

**Golden Rule**: If you're considering a change, ask yourself: "Is this absolutely necessary for continued operation?" If not, don't do it.

---

## Expected Maintenance Tasks

### Scenario 1: Operating System Updates

**When**: OS reaches end-of-life, security patches required

**Frequency**: Every 5-10 years

**Procedure**:
1. Test on non-production system first
2. Backup database
3. Update OS packages
4. Recompile binary if needed: `make clean && make`
5. Test application thoroughly
6. Deploy to production

**Why it might be needed**:
- Security vulnerabilities in OS
- Hardware requires newer OS
- glibc or system library updates

**Risk**: Medium. System is designed to be portable across POSIX systems.

---

### Scenario 2: SQLite Version Update

**When**: Critical security vulnerability in SQLite (extremely rare)

**Frequency**: Once per decade, if ever

**Procedure**:
1. Backup database file
2. Update libsqlite3-dev package
3. Test database integrity: `sqlite3 data/app.db "PRAGMA integrity_check;"`
4. Recompile application: `make clean && make`
5. Test thoroughly with backup database copy
6. Deploy

**Why it might be needed**:
- Security vulnerability (rare - SQLite has excellent track record)
- New features needed for performance

**Risk**: Low. SQLite maintains backward compatibility religiously.

---

### Scenario 3: Schema Migration

**When**: Business requirements change

**Frequency**: Rarely - avoid if possible

**Procedure**:

```bash
# 1. Backup database
cp data/app.db backups/app.db.pre-migration.$(date +%Y-%m-%d)

# 2. Create migration SQL file
cat > migrations/002_add_feature.sql << EOF
-- Migration: Add new feature
-- Date: 2035-03-15
-- Author: Admin

BEGIN TRANSACTION;

-- Add new column with default value (backward compatible)
ALTER TABLE accounts ADD COLUMN industry TEXT DEFAULT 'unspecified';

-- Update schema version
INSERT INTO schema_version (version, applied_at, description)
VALUES (2, strftime('%s', 'now'), 'Added industry to accounts');

COMMIT;
EOF

# 3. Test on copy of database
cp data/app.db data/app.db.test
sqlite3 data/app.db.test < migrations/002_add_feature.sql

# 4. Verify
sqlite3 data/app.db.test "PRAGMA integrity_check;"

# 5. Apply to production (during maintenance window)
sudo systemctl stop cero
sqlite3 data/app.db < migrations/002_add_feature.sql
sudo systemctl start cero

# 6. Monitor logs
tail -f logs/app.log
```

**Important Rules for Schema Changes**:
- NEVER drop columns (mark as deprecated instead)
- ALWAYS use DEFAULT values for new columns
- ALWAYS maintain backward compatibility
- NEVER change primary keys
- NEVER modify append-only tables (billing_events, audit_log)
- ALWAYS test on backup copy first

**Risk**: Medium-High. Schema changes are the most dangerous maintenance task.

---

### Scenario 4: TLS Certificate Renewal

**When**: Certificates expire

**Frequency**: Every 90 days (Let's Encrypt) or 1-2 years (paid certs)

**Procedure**:

```bash
# If using Let's Encrypt with certbot
sudo certbot renew

# Reload nginx (application doesn't need restart)
sudo systemctl reload nginx

# Verify
curl -I https://example.com/
```

**Note**: This is handled at the reverse proxy level, not in the application.

**Risk**: Very Low. No application changes needed.

---

### Scenario 5: Hardware Migration

**When**: Server hardware fails or needs replacement

**Frequency**: Every 5-10 years

**Procedure**:

```bash
# On old server:
# 1. Stop application
sudo systemctl stop cero

# 2. Backup everything
tar -czf cero-full-backup.tar.gz /opt/cero/

# 3. Copy to new server
scp cero-full-backup.tar.gz newserver:/tmp/

# On new server:
# 4. Extract backup
cd /opt
sudo tar -xzf /tmp/cero-full-backup.tar.gz

# 5. Install dependencies
sudo apt-get install build-essential libsqlite3-dev sqlite3

# 6. Compile binary (architecture may differ)
cd /opt/cero
make clean
make

# 7. Setup systemd service (see OPERATIONS.md)
sudo systemctl enable cero
sudo systemctl start cero

# 8. Verify
curl -I http://localhost:8080/
```

**Risk**: Low. Application is portable.

---

### Scenario 6: Security Vulnerability in Code

**When**: Vulnerability discovered in custom C code

**Frequency**: Hopefully never, but plan for it

**Procedure**:

1. **Assess impact**: Determine severity and affected components
2. **Develop patch**: Modify affected source code
3. **Test thoroughly**: Use backup database
4. **Compile new binary**: `make clean && make`
5. **Deploy**: Replace binary and restart

**Example - Hypothetical SQL Injection Fix**:

```c
// BEFORE (vulnerable)
sprintf(query, "SELECT * FROM users WHERE email = '%s'", email);

// AFTER (fixed)
sqlite3_prepare_v2(db, "SELECT * FROM users WHERE email = ?", -1, &stmt, NULL);
sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
```

**Risk**: Depends on vulnerability. Critical if auth/billing affected.

---

### Scenario 7: Dependency Update

**When**: Critical security issue in dependencies

**Frequency**: Rare - we have minimal dependencies

**Dependencies**:
- libc (glibc) - Updated with OS
- libsqlite3 - See Scenario 2
- libpthread - Updated with OS
- libcrypt/bcrypt - Updated with OS or may need manual update

**Procedure**:
1. Update system packages: `sudo apt-get update && sudo apt-get upgrade`
2. Recompile: `make clean && make`
3. Test
4. Deploy

**Risk**: Low. Standard system libraries are very stable.

---

## Maintenance Schedule

### Every 6 Months
- Review security mailing lists for SQLite and glibc
- Test backup restoration procedure
- Review and test disaster recovery plan

### Annually
- Review audit logs for unusual patterns
- Consider OS security updates
- Test application on latest OS version (non-production)
- Review this maintenance guide for updates

### Every 5 Years
- Consider OS upgrade to newer LTS version
- Review C code for deprecated functions
- Recompile with latest compiler for optimizations
- Hardware assessment

### Every 10 Years
- Expect major OS migration (e.g., Ubuntu 24.04 → 34.04)
- Potential hardware replacement
- Review if any fundamental technology has changed (unlikely)

---

## What NOT to Maintain

### Never Update These Unless Absolutely Necessary

**1. Framework or Library Migrations**
- We don't use frameworks. Don't add them.

**2. Database Engine Changes**
- SQLite is the database. Forever.
- Don't migrate to PostgreSQL, MySQL, MongoDB, etc.

**3. Programming Language Changes**
- C is the language. Don't rewrite in Go, Rust, Python, Node.js, etc.
- Rewriting introduces bugs and defeats longevity goal.

**4. Frontend Framework Adoption**
- No React, Vue, Angular, etc.
- Server-rendered HTML is the frontend.

**5. "Modernization" Projects**
- Resist urge to modernize for sake of modernization
- Modern != better for longevity

**6. Automated Billing Systems**
- Manual billing is intentional
- Don't integrate Stripe webhooks, payment APIs, etc.

**7. Cloud-Specific Features**
- No AWS/Azure/GCP-specific code
- Keep portable

**8. Microservices**
- This is a monolith. Keep it that way.

**9. Real-time Features**
- No WebSockets, Server-Sent Events, etc.
- Request-response model only

**10. Background Job Systems**
- No queues, workers, schedulers
- Everything on-demand

---

## Code Modification Guidelines

If you must modify the code:

### DO:
- Write clear, simple C code
- Use standard library functions
- Document why the change was necessary
- Test extensively
- Keep changes minimal
- Maintain backward compatibility
- Follow existing code style
- Add comments explaining non-obvious logic

### DON'T:
- Add dependencies
- Use compiler-specific extensions
- Write "clever" code
- Optimize prematurely
- Add features "just in case"
- Change working code without strong reason
- Remove comments or documentation

---

## Disaster Scenarios & Recovery

### Scenario: Database Corruption Beyond Repair

**Recovery**:
1. Restore from most recent backup
2. Contact customers about potential data loss
3. Manually reconstruct critical data if possible
4. Review what caused corruption to prevent recurrence

**Prevention**:
- Regular automated backups (daily)
- Off-site backup storage
- Periodic backup restoration tests

### Scenario: Complete Data Center Failure

**Recovery**:
1. Provision new server
2. Install OS and dependencies
3. Restore from off-site backup
4. Compile and deploy binary
5. Update DNS

**Expected Downtime**: 2-8 hours

**Prevention**:
- Maintain off-site backups
- Document recovery procedures
- Test annually

### Scenario: All Backups Lost

**Recovery**:
This is catastrophic. Prevention is critical.

**Prevention**:
- Multiple backup locations (local, off-site, cloud)
- Different backup methods (file copy, SQL dump)
- Regular testing of backup integrity
- Backup of backup procedures

---

## When to Consider Replacement

The system should be replaced if:

1. **C language becomes unsupported** (extremely unlikely in next 50 years)
2. **SQLite becomes unsupported** (extremely unlikely)
3. **HTTP protocol fundamentally changes** (unlikely)
4. **POSIX systems cease to exist** (extremely unlikely)
5. **Business model changes so drastically** that system can't adapt

Note: None of these are expected within 50 year timeframe.

---

## Maintenance Philosophy Checklist

Before performing any maintenance, ask:

- [ ] Is this absolutely necessary for continued operation?
- [ ] What happens if we don't do this?
- [ ] Is this change backward compatible?
- [ ] Have we tested on a non-production system?
- [ ] Have we backed up all data?
- [ ] Can we easily rollback if something goes wrong?
- [ ] Does this align with the longevity goals?
- [ ] Are we adding complexity or removing it?

If you answer "no" to any of these, reconsider the maintenance.

---

## Version Control for Changes

Document all changes:

```bash
# Create a changes log
echo "$(date): Brief description of change" >> CHANGELOG.md
echo "Reason: Why this change was necessary" >> CHANGELOG.md
echo "Impact: What was affected" >> CHANGELOG.md
echo "Tested: How it was tested" >> CHANGELOG.md
echo "" >> CHANGELOG.md
```

Keep this log forever. Future maintainers will thank you.

---

## Final Words

This system was designed to be boring, simple, and reliable. The best maintenance is no maintenance.

When in doubt:
1. **Don't change it**
2. **If you must change it, change as little as possible**
3. **If you must change more, question your assumptions**

The goal is for this system to still be running, largely unchanged, in 2075.

**Resist the urge to "improve" things.**

**Boring is beautiful.**

**Simple is sustainable.**

**Manual is maintainable.**
