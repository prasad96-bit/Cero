# System Architecture Overview

## 1. Core Philosophy

This system is designed to run for 50+ years with near-zero maintenance.
Every decision prioritizes:

- **Longevity** over convenience
- **Simplicity** over automation
- **Understandability** over abstraction
- **Auditability** over efficiency
- **Manual control** over automation

## 2. System Architecture

### 2.1 High-Level Architecture

```
┌─────────────────────────────────────────────────┐
│              HTTP Clients (Browsers)            │
└──────────────────┬──────────────────────────────┘
                   │ HTTP/1.1
┌──────────────────▼──────────────────────────────┐
│          C HTTP Server (Single Binary)          │
│  ┌───────────────────────────────────────────┐  │
│  │  Request Router                           │  │
│  └───────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────┐  │
│  │  Authentication & Session Management      │  │
│  └───────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────┐  │
│  │  Entitlement Engine                       │  │
│  └───────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────┐  │
│  │  Business Logic                           │  │
│  │  - Billing (Manual)                       │  │
│  │  - Reports (On-Demand)                    │  │
│  │  - Rate Limiting                          │  │
│  └───────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────┐  │
│  │  Template Engine (Mustache-style)         │  │
│  └───────────────────────────────────────────┘  │
└──────────────────┬──────────────────────────────┘
                   │ SQLite3 C API
┌──────────────────▼──────────────────────────────┐
│              SQLite Database                    │
│         (Single File, WAL Mode)                 │
│                                                 │
│  Tables:                                        │
│  - accounts                                     │
│  - users                                        │
│  - sessions                                     │
│  - subscriptions (authoritative)                │
│  - billing_events (append-only)                 │
│  - rate_limits                                  │
│  - audit_log (append-only)                      │
└─────────────────────────────────────────────────┘
```

### 2.2 Key Architectural Decisions

#### Single Binary
- Entire application compiles to one executable
- No runtime dependencies beyond libc and SQLite
- Deployment = copy binary + config + database

#### SQLite as System of Record
- No external databases
- No network dependencies
- Transactions guarantee consistency
- File-based backups are trivial
- WAL mode for better concurrency
- Foreign keys enforced

#### Server-Rendered HTML
- No JavaScript required for core functionality
- Works in any browser from the last 30 years
- Simple to audit and understand
- No build step for frontend

#### Manual Billing Model ("Dumb Payment Links")
- Payment happens outside the application
- Admin manually confirms payments
- No runtime dependency on payment providers
- Subscriptions degrade gracefully
- Grace periods built-in
- Payment provider outages have zero impact

#### No Background Jobs
- All computation is on-demand
- No schedulers, queues, or workers
- Simpler operational model
- Easier to reason about state

#### Append-Only Audit Tables
- Billing events never deleted
- Audit log never deleted
- Enables forensic analysis
- Simplifies compliance

## 3. Component Architecture

### 3.1 HTTP Server
- Custom C HTTP/1.1 server
- Single-threaded or thread-per-request model
- Keep-alive support
- Request parsing and routing
- Cookie-based sessions
- CSRF protection via form tokens

### 3.2 Authentication & Authorization
- Email + password (bcrypt hashed)
- Server-side sessions stored in SQLite
- Session tokens in secure cookies
- Role-based access (user, admin)
- No OAuth, no external identity providers

### 3.3 Subscription & Entitlement Engine
- Subscriptions table is authoritative
- Plans: free, pro, enterprise (static enum)
- Status: active, grace_period, expired, cancelled
- Entitlements computed deterministically from subscription state
- No background reconciliation

### 3.4 Billing System
- Manual workflow:
  1. Customer requests upgrade
  2. System shows payment link or invoice instructions
  3. Customer pays externally
  4. Admin confirms payment manually
  5. Admin updates subscription in database
- All billing events logged in append-only table
- Grace periods configurable per account

### 3.5 Reports Engine
- On-demand computation (no caching)
- Date range filtering
- Simple grouping (by day, week, month)
- CSV export
- Access control based on plan:
  - Free: 7-day preview
  - Paid: unlimited range, grouping, export

### 3.6 Rate Limiting
- Stored in SQLite (no Redis)
- Per-IP and per-user limits
- Token bucket algorithm
- Cleanup of old entries on each request

### 3.7 Template Engine
- Simple Mustache-style substitution
- Syntax: `{{variable_name}}`
- No logic in templates
- All data prepared in C code
- Templates are plain HTML files

## 4. Data Flow

### 4.1 Request Flow
```
1. HTTP Request arrives
2. Parse HTTP headers and body
3. Extract session cookie
4. Validate session (query SQLite)
5. Check rate limit (query SQLite)
6. Route to handler function
7. Check entitlements
8. Execute business logic
9. Prepare template data
10. Render HTML template
11. Send HTTP response
12. Log request (append to log file)
```

### 4.2 Authentication Flow
```
Login:
1. User submits email + password
2. Query users table by email
3. Verify password with bcrypt
4. Create session record in sessions table
5. Set session cookie
6. Redirect to dashboard

Session Validation:
1. Extract session_token from cookie
2. Query sessions table
3. Check expiration timestamp
4. Load user + account data
5. Attach to request context
```

### 4.3 Billing Flow
```
Upgrade Request:
1. User clicks "Upgrade to Pro"
2. System generates payment instructions page
3. Shows payment link or invoice details
4. User pays externally

Payment Confirmation (Admin):
1. Admin navigates to billing admin UI
2. Searches for account
3. Clicks "Mark as Paid"
4. Selects plan and duration
5. System updates subscriptions table
6. System appends to billing_events table
7. User's entitlements update immediately
```

### 4.4 Report Generation Flow
```
1. User requests report with parameters
2. Check entitlements (date range limits)
3. Build SQL query with date filters
4. Execute query synchronously
5. If CSV: stream rows to CSV format
6. If HTML: render table with template
7. Return response
```

## 5. Failure Modes & Degradation

### 5.1 Payment Provider Outage
- **Impact**: None
- **Behavior**: Admin continues manual billing workflow
- **Recovery**: N/A (system never depended on provider)

### 5.2 Database Corruption
- **Impact**: Service down
- **Behavior**: Restore from backup
- **Recovery**: Stop server, restore DB file, restart server

### 5.3 Admin Delayed in Billing
- **Impact**: Minimal (grace period applies)
- **Behavior**: Users retain access during grace period
- **Recovery**: Admin processes billing when available

### 5.4 Server Crash
- **Impact**: Temporary downtime
- **Behavior**: Sessions persist in database
- **Recovery**: Restart binary

### 5.5 Network Partition
- **Impact**: Users cannot access service
- **Behavior**: Local state remains consistent
- **Recovery**: Network restored, service resumes

## 6. Security Model

### 6.1 Authentication
- Passwords hashed with bcrypt (cost factor 12)
- Session tokens are cryptographically random
- Sessions expire after 30 days of inactivity
- No password reset without manual admin intervention

### 6.2 Authorization
- All queries scoped by account_id
- Foreign keys prevent data leakage
- Admin role required for billing operations
- Rate limiting prevents abuse

### 6.3 Input Validation
- All user input validated before database insertion
- SQL injection prevented by parameterized queries
- XSS prevented by HTML escaping in templates
- CSRF prevented by form tokens

### 6.4 Transport Security
- Designed to run behind reverse proxy (nginx)
- TLS termination at proxy layer
- Secure cookie flags set (HttpOnly, Secure, SameSite)

## 7. Operational Model

### 7.1 Deployment
```
1. Compile binary on build machine
2. Copy binary to server
3. Copy config file
4. Initialize SQLite database (if new)
5. Start process
6. Configure reverse proxy
```

### 7.2 Backups
```
Daily:
1. Stop writes (or use WAL checkpoint)
2. Copy database file
3. Copy config and secrets
4. Store off-machine
5. Resume writes

No hot backup required - downtime acceptable for durability
```

### 7.3 Monitoring
```
- Check process is running
- Check log file for errors
- Check database file size growth
- Check disk space
- Optional: ping endpoint for uptime

No complex monitoring infrastructure required
```

### 7.4 Maintenance
```
Expected tasks (annual):
- Review and rotate logs
- Test backup restoration
- Review audit log for anomalies

Expected tasks (rare):
- Update TLS certificates (at proxy level)
- Server hardware replacement
- Binary recompilation for OS changes
```

## 8. Technology Stack

| Layer | Technology | Rationale |
|-------|------------|-----------|
| Language | C (C99) | Stable, portable, minimal dependencies |
| Database | SQLite 3 | Embedded, stable, widely understood |
| HTTP Server | Custom | Full control, no dependencies |
| Crypto | bcrypt (via library) | Long-lived algorithm |
| Templates | Custom Mustache-style | Simple, no dependencies |
| Logging | Plain text files | Human-readable, standard tools |
| Config | Plain text (key=value) | Editable with any text editor |

## 9. Scalability Model

### 9.1 Vertical Scaling
- Single server model
- Designed for thousands of users, not millions
- SQLite performs well under moderate load
- If needed: increase server RAM and CPU

### 9.2 Horizontal Scaling (Not Supported)
- No built-in support for multiple instances
- No distributed database
- If needed in future: reverse proxy can load balance read-only replicas

### 9.3 Performance Characteristics
- Request latency: 10-100ms (database-bound)
- Throughput: 100-1000 req/sec (depends on hardware)
- Database size: GB to low TB range
- Concurrency: Limited by SQLite write locks

## 10. Longevity Considerations

### 10.1 Why This Will Last 50+ Years

1. **C is stable**: C99 will be supported for decades
2. **SQLite is stable**: Commitment to backward compatibility
3. **HTTP/1.1 is stable**: Not going away
4. **HTML is stable**: Core HTML works everywhere
5. **No external dependencies**: Nothing to break when vendors disappear
6. **No automation to maintain**: No jobs, queues, or schedulers to manage
7. **Simple mental model**: Easy to understand decades from now
8. **Append-only audit**: Full history preserved
9. **File-based backup**: Trivial disaster recovery

### 10.2 What Could Require Changes

1. **OS changes**: May need recompilation (rare)
2. **TLS protocol changes**: Handled at reverse proxy
3. **Compliance requirements**: May need schema additions
4. **Business model changes**: May need new plan types

### 10.3 Upgrade Strategy

- In-place schema migrations using SQLite ALTER TABLE
- Backward-compatible schema changes only
- Version table tracks schema version
- Downtime acceptable for major upgrades

## 11. Non-Goals

This system intentionally does NOT support:

- Real-time features
- WebSockets
- Mobile apps
- APIs (beyond HTML responses)
- OAuth or SSO
- Automated recurring billing
- Background jobs
- Webhooks
- Microservices
- Event sourcing
- CQRS
- Graph APIs
- Usage-based billing
- Dynamic pricing
- A/B testing infrastructure
- Analytics beyond basic reports

Adding any of these would compromise the longevity goal.

## 12. Success Metrics

This architecture succeeds if:

1. System runs for years between code changes
2. A single person can operate it indefinitely
3. Total operational cost remains flat
4. Billing can be managed manually without friction
5. Recovery from any failure takes < 1 hour
6. New developer understands system in < 1 week
7. Customers never locked out due to external dependencies
8. Source code remains readable and obvious
