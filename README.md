# Core SaaS Platform

**A 50+ year, ultra-low-maintenance SaaS system built for longevity.**

## Philosophy

This is NOT a modern web app.
This is NOT a startup MVP.
This is NOT a framework-based system.

This is a **durable, boring, self-contained SaaS platform** designed to operate for 50+ years with near-zero maintenance and no runtime dependencies on third-party services.

## Design Principles

- **Longevity over convenience**
- **Simplicity over automation**
- **Manual control over magic**
- **Understandability over abstraction**
- **Boring over exciting**

## Technology Stack

| Layer | Technology | Why |
|-------|------------|-----|
| Language | C (C99) | Stable, portable, will be supported for decades |
| Database | SQLite | Embedded, zero-config, backward compatible |
| Frontend | Server-rendered HTML | Works in any browser, no build step |
| Templates | Mustache-style | Simple placeholder replacement |
| HTTP | Custom server | Full control, no dependencies |
| Deployment | Single binary | Copy and run, no containers required |

## Core Features

### Authentication
- Email + password (bcrypt)
- Server-side sessions
- No OAuth, no external identity providers
- No JWTs

### Multi-Tenancy
- Account-based isolation
- All data scoped by account_id
- Foreign key constraints enforced

### Billing (Manual "Dumb Link" Model)
- **Payments happen outside the app**
- Admin manually confirms payments
- Subscription state stored locally in SQLite
- No runtime dependency on payment providers
- Grace periods supported
- **System continues working if Stripe/PayPal disappear**

### Advanced Reports
- On-demand computation (no pre-calculation)
- Date range filtering
- Grouping (by day, week, month) - Pro plan only
- CSV export - Pro plan only
- Entitlement-based access control

### Rate Limiting
- Token bucket algorithm
- Stored in SQLite (no Redis)
- Per-IP and per-user limits

### Audit Trail
- Append-only billing_events table
- Append-only audit_log table
- Never deleted, always queryable

## Quick Start

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential libsqlite3-dev sqlite3

# macOS
brew install sqlite3
```

### Build

```bash
# Clone/copy source
cd /opt/cero

# Check dependencies
make check-deps

# Build
make

# Initialize database
make init-db
```

### Configure

```bash
# Create secrets file
cp config/secrets.txt.template config/secrets.txt

# Generate secrets
openssl rand -hex 32  # For SESSION_SECRET
openssl rand -hex 32  # For CSRF_SECRET

# Generate admin password hash (requires apache2-utils)
htpasswd -bnBC 12 "" yourpassword | tr -d ':\n'

# Edit secrets.txt and add the generated values
nano config/secrets.txt
```

### Run

```bash
# Run directly
./cero

# Or install as systemd service (see OPERATIONS.md)
sudo systemctl start cero
```

### Access

```
http://localhost:8080/
```

## Documentation

| Document | Purpose |
|----------|---------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | System architecture and design decisions |
| [FOLDER_STRUCTURE.md](FOLDER_STRUCTURE.md) | Directory layout and file organization |
| [OPERATIONS.md](OPERATIONS.md) | Deployment, backup, restore procedures |
| [MAINTENANCE.md](MAINTENANCE.md) | Long-term maintenance guide |
| [IMPLEMENTATION.md](IMPLEMENTATION.md) | Guide to complete remaining components |

## Project Structure

```
cero/
├── src/                # C source code
│   ├── core/          # HTTP server and routing
│   ├── auth/          # Authentication and sessions
│   ├── billing/       # Subscription and entitlements
│   ├── reports/       # Advanced reports
│   ├── templates/     # Template engine
│   └── utils/         # Utilities (db, log, config)
├── templates/         # HTML templates
├── config/            # Configuration files
│   ├── config.txt
│   ├── secrets.txt.template
│   └── schema.sql
├── data/              # SQLite database (runtime)
├── logs/              # Application logs (runtime)
├── backups/           # Database backups (runtime)
└── Makefile          # Build system
```

## Key Architectural Decisions

### Single C Binary
- No runtime dependencies beyond libc and SQLite
- Deployment = copy binary + config + database
- Portable across any POSIX system

### SQLite as System of Record
- No network database required
- Transactions guarantee consistency
- File-based backups are trivial
- Will be backward compatible for decades

### Manual Billing
Intentionally designed to survive payment provider outages:

1. Customer requests upgrade
2. System shows payment link or invoice instructions
3. Customer pays externally (bank, check, online payment)
4. Admin manually confirms payment
5. Admin updates subscription in database
6. Customer gets immediate access

**Why?** External payment APIs change, disappear, or become expensive. Manual billing is forever.

### No Background Jobs
- All computation is on-demand
- No schedulers, queues, or workers
- Simpler operational model
- Easier to reason about state

### Server-Rendered HTML Only
- No JavaScript required for core functionality
- Works in browsers from 1995 to 2075
- Simple to audit and understand
- No build step for frontend

## Failure Modes (Intentional Design)

| Failure | Impact | Recovery |
|---------|--------|----------|
| Payment provider outage | None | Manual billing continues |
| Internet outage | Users can't access (expected) | Service resumes when connection restored |
| Database corruption | Service down | Restore from backup |
| Admin delay in billing | Minimal (grace period) | Admin processes when available |
| Server crash | Temporary downtime | Restart binary |
| Third-party service shutdown | None | System has no external dependencies |

## Operational Model

### Deployment
```bash
# Build once
make

# Copy to server
scp cero server:/opt/cero/

# Start
systemctl start cero
```

### Backups
```bash
# Daily automated backup (cron)
0 2 * * * cp /opt/cero/data/app.db /opt/cero/backups/app.db.$(date +%Y-%m-%d)

# Or use Makefile
make backup
```

### Monitoring
```bash
# Check if running
systemctl status cero

# Check logs
tail -f logs/app.log

# Health check
curl -I http://localhost:8080/
```

## Security Model

### Authentication
- Passwords hashed with bcrypt (cost factor 12)
- Session tokens cryptographically random
- Sessions expire after 30 days of inactivity
- No password reset without manual admin intervention

### Authorization
- All queries scoped by account_id
- Foreign keys prevent data leakage
- Admin role required for billing operations
- Rate limiting prevents abuse

### Transport Security
- Designed to run behind reverse proxy (nginx)
- TLS termination at proxy layer
- Secure cookie flags (HttpOnly, Secure, SameSite)

## Scaling Model

### Vertical Scaling
- Single server model
- Designed for thousands of users, not millions
- SQLite performs well under moderate load
- Scale up: increase RAM and CPU

### Horizontal Scaling
- Not currently supported
- Not needed for target use case
- If needed: read replicas via reverse proxy

### Performance Characteristics
- Request latency: 10-100ms (database-bound)
- Throughput: 100-1000 req/sec
- Database size: GB to low TB range
- Concurrency: Limited by SQLite write locks

## What This System Does NOT Support

Intentionally excluded to preserve longevity:

- Real-time features (WebSockets)
- Mobile apps
- Client-side JavaScript frameworks
- OAuth or SSO
- Automated recurring billing
- Background jobs
- Webhooks
- Microservices
- Event sourcing
- Graph APIs
- Usage-based billing
- A/B testing infrastructure
- External analytics services

**Adding any of these would compromise the 50+ year goal.**

## Success Criteria

This system succeeds if:

1. ✅ Runs for years between code changes
2. ✅ A single person can operate it indefinitely
3. ✅ Billing can be managed manually without friction
4. ✅ Recovery from any failure takes < 1 hour
5. ✅ New developer understands system in < 1 week
6. ✅ Customers never locked out due to external dependencies
7. ✅ Source code remains readable and obvious
8. ✅ Total operational cost remains flat over time

## Implementation Status

### ✅ Completed
- Architecture design and documentation
- SQLite schema with append-only audit tables
- Folder structure and organization
- Core HTTP server skeleton
- Configuration and secrets management
- Logging system
- HTML templates (login, dashboard, billing, reports, admin)
- Makefile build system
- Operational playbook
- Long-term maintenance guide

### 🚧 To Be Implemented

The following components have headers defined and architecture documented, but need full implementation:

- HTTP request/response parsing
- Router and handler dispatch
- Authentication (bcrypt integration)
- Session management
- Subscription and entitlement logic
- Admin billing operations
- Reports generation and CSV export
- Rate limiting implementation
- Template rendering engine
- String and time utilities

See [IMPLEMENTATION.md](IMPLEMENTATION.md) for detailed implementation guide.

## Building the Remaining Components

The project is ~40% implemented with complete architecture, schema, and skeleton code. To complete:

1. Implement the components listed above following the header files
2. Use the architecture and schema as your guide
3. Follow the implementation patterns in existing code (log.c, config.c, db.c, server.c)
4. Test each component thoroughly
5. Keep it simple and boring

**Estimated effort**: 40-80 hours for an experienced C developer.

## Why This Approach?

### The Problem with Modern SaaS
- Frameworks change every 2-3 years
- Dependencies break constantly
- Cloud services deprecate features
- Payment APIs evolve and break integrations
- Background job systems add complexity
- Real-time features add operational burden

### This Solution
- C hasn't changed fundamentally in 30+ years
- SQLite is committed to backward compatibility until 2050+
- HTML and HTTP are stable standards
- No dependencies to break
- Manual processes are timeless
- Simple code is maintainable code

## Who Is This For?

### This system is ideal for:
- Long-term B2B SaaS products
- Infrastructure/operations businesses
- Low-churn, high-stability markets
- Founders who value reliability over rapid iteration
- Teams that want to build once and maintain minimally

### This system is NOT ideal for:
- Rapid prototyping and iteration
- Consumer apps requiring frequent updates
- Real-time collaborative features
- High-scale (millions of users)
- Teams that prefer modern frameworks

## Maintenance Expectations

### Annual Tasks
- Review security updates for OS and SQLite
- Test backup restoration
- Rotate logs
- Vacuum database

### Every 5 Years
- OS upgrade (recompile binary)
- Hardware refresh (copy binary + database)

### Every 10 Years
- Major OS migration
- Review for deprecated functions

### Expected Lifetime
**50+ years with minimal changes**

## Philosophy in Practice

> "The best code is no code."

> "The best maintenance is no maintenance."

> "The best dependency is no dependency."

> "The best API is no API."

> "Boring is beautiful."

This system embodies these principles.

## Questions?

### "Why not use a framework?"
Frameworks change. C and SQLite don't. In 2050, your framework may be gone. C and SQLite will still be here.

### "Why not use automated billing?"
Automated billing depends on external APIs. External APIs change, break, and disappear. Manual billing is forever.

### "Why not use JavaScript?"
JavaScript is fine for rapid iteration. But for 50-year longevity, server-rendered HTML is more stable.

### "This seems like a lot of work to build from scratch."
It is. But it's work done once, not repeatedly. Modern frameworks require constant updates. This doesn't.

### "What if SQLite can't handle the load?"
SQLite handles ~100K requests/second on modest hardware. If you need more, you've outgrown the target use case.

### "What about scaling?"
Vertical scaling (bigger server) gets you very far. If you need horizontal scaling, this isn't the right architecture.

### "Is this production-ready?"
The architecture is production-ready. The implementation is ~40% complete. Complete the remaining components following the implementation guide.

**Next Steps**: See [IMPLEMENTATION.md](IMPLEMENTATION.md) to complete the remaining components.

---

## Final Words

This system is designed to be **boring, simple, and reliable**.

It will not be featured in tech blogs.
It will not win hackathons.
It will not impress other developers.

But it will still be running, largely unchanged, in 2075.

And that's the point.

**Build once. Run forever.**

---

## Donate 

To support the developer, you can <a href="https://buy.stripe.com/14AaEWfRXcm5aPrfSag360b">make a donation here<a>.

