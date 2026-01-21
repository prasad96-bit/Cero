# Folder & File Structure

## Directory Layout

```
cero/
├── src/                        # C source code
│   ├── core/                   # Core HTTP server and routing
│   │   ├── server.c           # HTTP server implementation
│   │   ├── server.h
│   │   ├── router.c           # URL routing
│   │   ├── router.h
│   │   ├── request.c          # HTTP request parsing
│   │   ├── request.h
│   │   └── response.c         # HTTP response building
│   │       └── response.h
│   ├── auth/                   # Authentication and sessions
│   │   ├── auth.c             # Login, logout, password verification
│   │   ├── auth.h
│   │   ├── session.c          # Session management
│   │   ├── session.h
│   │   ├── bcrypt.c           # Bcrypt password hashing (or use library)
│   │   └── bcrypt.h
│   ├── billing/                # Subscription and billing logic
│   │   ├── subscription.c     # Subscription state management
│   │   ├── subscription.h
│   │   ├── entitlement.c      # Entitlement checking
│   │   ├── entitlement.h
│   │   ├── admin.c            # Admin billing operations
│   │   └── admin.h
│   ├── reports/                # Advanced reports feature
│   │   ├── reports.c          # Report generation
│   │   ├── reports.h
│   │   ├── csv.c              # CSV export
│   │   └── csv.h
│   ├── templates/              # Template rendering
│   │   ├── template.c         # Mustache-style template engine
│   │   └── template.h
│   ├── utils/                  # Utility functions
│   │   ├── db.c               # SQLite wrapper functions
│   │   ├── db.h
│   │   ├── log.c              # Logging
│   │   ├── log.h
│   │   ├── config.c           # Configuration parsing
│   │   ├── config.h
│   │   ├── ratelimit.c        # Rate limiting
│   │   ├── ratelimit.h
│   │   ├── string_utils.c     # String utilities
│   │   ├── string_utils.h
│   │   ├── time_utils.c       # Time/date utilities
│   │   └── time_utils.h
│   └── main.c                  # Application entry point
│
├── templates/                  # HTML templates
│   ├── layout.html            # Base layout template
│   ├── login.html             # Login page
│   ├── dashboard.html         # User dashboard
│   ├── billing.html           # Billing page
│   ├── reports.html           # Reports page
│   ├── reports_preview.html   # Reports preview (free users)
│   ├── admin_billing.html     # Admin billing interface
│   └── error.html             # Error page template
│
├── config/                     # Configuration and secrets
│   ├── config.txt             # Application configuration
│   ├── secrets.txt            # Secrets (not in git)
│   └── schema.sql             # Database schema definition
│
├── logs/                       # Application logs
│   └── app.log                # Current log file (rotated daily)
│
├── backups/                    # Database backups
│   └── .gitkeep
│
├── data/                       # Runtime data
│   └── app.db                 # SQLite database file
│
├── docs/                       # Documentation
│   ├── ARCHITECTURE.md        # System architecture (already created)
│   ├── FOLDER_STRUCTURE.md    # This file
│   ├── OPERATIONS.md          # Operational playbook
│   └── MAINTENANCE.md         # Long-term maintenance guide
│
├── Makefile                    # Build system
├── README.md                   # Project overview
└── .gitignore                  # Git ignore file
```

## File Descriptions

### Source Code (`src/`)

#### `src/main.c`
- Application entry point
- Initializes database connection
- Loads configuration
- Starts HTTP server
- Sets up signal handlers for graceful shutdown

#### `src/core/`
Core HTTP server and request handling.

**server.c/h**
- HTTP/1.1 server implementation
- Socket management
- Request acceptance and dispatching
- Keep-alive support

**router.c/h**
- URL pattern matching
- Route registration
- Handler dispatch
- Static route table (no dynamic routing)

**request.c/h**
- HTTP request parsing
- Header parsing
- Cookie extraction
- Form data parsing (POST bodies)

**response.c/h**
- HTTP response building
- Header generation
- Cookie setting
- Status code handling

#### `src/auth/`
Authentication and session management.

**auth.c/h**
- User authentication (email + password)
- Password verification with bcrypt
- Login/logout handlers
- User registration (if enabled)

**session.c/h**
- Session creation
- Session validation
- Session expiration
- Session token generation (cryptographically secure)

**bcrypt.c/h**
- Bcrypt password hashing
- Alternatively: link against libbcrypt or use OpenBSD's implementation

#### `src/billing/`
Subscription and billing logic.

**subscription.c/h**
- Get subscription status for account
- Check if subscription is valid
- Apply grace period logic
- Subscription state transitions

**entitlement.c/h**
- Check if account has access to feature
- Plan-based entitlement rules
- Feature flags based on subscription

**admin.c/h**
- Admin UI handlers
- Mark subscription as paid
- Update subscription details
- View billing history

#### `src/reports/`
Advanced reports feature.

**reports.c/h**
- Generate reports with filters
- Date range validation
- SQL query building
- Entitlement checking for report features

**csv.c/h**
- CSV formatting
- Escaping special characters
- Streaming CSV output

#### `src/templates/`
Template rendering engine.

**template.c/h**
- Load template files
- Parse template syntax
- Replace `{{variable}}` placeholders
- Escape HTML entities
- Simple conditionals (optional)

#### `src/utils/`
Utility and helper functions.

**db.c/h**
- SQLite connection management
- Prepared statement helpers
- Transaction management
- Schema initialization
- Database backup functions

**log.c/h**
- Log to file
- Log levels (DEBUG, INFO, WARN, ERROR)
- Log rotation
- Thread-safe logging (if multi-threaded)

**config.c/h**
- Parse config.txt
- Parse secrets.txt
- In-memory configuration structure
- Configuration validation

**ratelimit.c/h**
- Rate limit checking
- Token bucket algorithm
- Per-IP and per-user limits
- SQLite-based state storage
- Cleanup of expired rate limit records

**string_utils.c/h**
- String manipulation helpers
- URL encoding/decoding
- HTML entity escaping
- Safe string copying

**time_utils.c/h**
- Current Unix timestamp
- ISO 8601 formatting
- Date range calculations
- Timezone handling (always UTC)

### Templates (`templates/`)

All templates are plain HTML with `{{variable}}` placeholders.

**layout.html**
- Base HTML structure
- Common header and footer
- Navigation
- Placeholders for page content

**login.html**
- Login form
- Error messages
- CSRF token field

**dashboard.html**
- User dashboard
- Account information
- Subscription status
- Quick links

**billing.html**
- Current subscription details
- Payment instructions
- Payment link (if applicable)
- Subscription history

**reports.html**
- Report generation form
- Date range pickers
- Grouping options
- CSV export button
- Report results table

**reports_preview.html**
- Limited report preview for free users
- Upgrade prompt
- Sample data only

**admin_billing.html**
- Account search
- Subscription management table
- Mark as paid form
- Billing event log

**error.html**
- Generic error page
- Error code and message
- Link back to dashboard

### Configuration (`config/`)

**config.txt**
```
# Server configuration
PORT=8080
HOST=0.0.0.0

# Database
DB_PATH=data/app.db

# Logging
LOG_PATH=logs/app.log
LOG_LEVEL=INFO

# Rate limiting
RATE_LIMIT_REQUESTS_PER_MINUTE=60

# Session
SESSION_EXPIRY_SECONDS=2592000  # 30 days
```

**secrets.txt**
```
# Secrets (do not commit to git)
SESSION_SECRET=<random_hex_string>
CSRF_SECRET=<random_hex_string>
ADMIN_PASSWORD_HASH=<bcrypt_hash>
```

**schema.sql**
- SQLite schema definition
- CREATE TABLE statements
- CREATE INDEX statements
- Initial data (if any)

### Logs (`logs/`)

**app.log**
- Plain text log file
- Rotated daily (app.log.2026-01-21)
- Format: `[TIMESTAMP] [LEVEL] [MODULE] Message`

Example:
```
[2026-01-21T12:34:56Z] [INFO] [server] Server started on port 8080
[2026-01-21T12:35:01Z] [DEBUG] [auth] Login attempt for user@example.com
[2026-01-21T12:35:01Z] [INFO] [auth] Login successful for user@example.com
```

### Backups (`backups/`)

Contains database backup files:
```
backups/
├── app.db.2026-01-20
├── app.db.2026-01-19
└── app.db.2026-01-18
```

### Data (`data/`)

**app.db**
- SQLite database file
- SQLite WAL files (.db-wal, .db-shm)

### Documentation (`docs/`)

**ARCHITECTURE.md**
System architecture and design decisions.

**OPERATIONS.md**
Operational procedures:
- Deployment
- Backups
- Restoration
- Monitoring

**MAINTENANCE.md**
Long-term maintenance guide:
- Expected maintenance tasks
- Schema migration procedures
- Recompilation procedures

### Build System

**Makefile**
```makefile
CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -O2
LDFLAGS=-lsqlite3 -lpthread -lm

SRC=$(wildcard src/**/*.c src/*.c)
OBJ=$(SRC:.c=.o)
TARGET=cero

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

install:
	cp $(TARGET) /usr/local/bin/

.PHONY: all clean install
```

## Design Rationale

### Why This Structure?

1. **Flat module hierarchy**: Only 2 levels deep, easy to navigate
2. **Separation by concern**: Each directory has a clear purpose
3. **No excessive abstraction**: Modules map directly to features
4. **Templates separate from code**: Easy to modify without recompiling
5. **Config in plain text**: No JSON, YAML, or custom formats
6. **Logs human-readable**: Standard text, no binary formats
7. **Backups self-explanatory**: Just database file copies

### What's Not Here?

Intentionally excluded:

- No `node_modules/`
- No `vendor/` or dependency directories
- No `.env` files (using plain text config)
- No Docker or container configs
- No CI/CD configs
- No test directories (tests can be added later if needed)
- No migration directories (migrations in schema.sql comments)
- No static asset build pipeline

### Adding New Features

To add a new feature:

1. Create new C files in appropriate `src/` subdirectory
2. Add route handlers in `router.c`
3. Create HTML template in `templates/`
4. Update schema.sql if database changes needed
5. Recompile with `make`

Example: Adding a "notifications" feature:
```
src/notifications/
├── notifications.c
├── notifications.h
templates/
├── notifications.html
```

No framework to extend, no generators to run, just files.
