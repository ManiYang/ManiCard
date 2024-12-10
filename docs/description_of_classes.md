# Brief Description of Selected Classes

### `Neo4jHttpApiClient` and `Neo4jTransaction`

Accesses Neo4j DB by invoking the DB's HTTP API.

Currently there is no time-out and retry mechanism. A retry mechanism will need to take care of write operations that is not idempotent.

### `CardsDataAccess` and `BoardsDataAccess`

CRUD of cards, relationships, and boards.

### `QueuedDbAccess`

A proxy of `CardsDataAccess` and `BoardsDataAccess`.

The CRUD operations are queued (FIFO). An operation is performed only after the previous one gets response. 

If a write operation fails, all following operations will fail directly without being performed.

This is a (probably suboptimal) way to ensure causal consistency.

### `DebouncedDbAccess`

Limits the frequency of writing to DB when, for example, the user is editing the textual contents of a card.

### `PersistedDataAccess`

Accesses data stored in DB and local files. Also manages caches of some of the data.

### `AppData`

- Accesses persisted data.
- Emits signals about data updates. 
- It can help propagate updates of common states among UI widgets.
- In the future this class can be modified to support an undo/redo mechanism.

### `Services`

- Creates and sets up long-lived objects on app start.
- Provides access to the instance of `AppData` for UI classes.

### `BoardView`, `NodeRect`, and `EdgeArrow`

Class `BoardView` 
- has an instance of `QGraphicsView` and an instance of `GraphicsScene` (which inherits `QGraphicsScene`),
- manages a collection of `NodeRect` objects and a collection of `EdgeArrow` objects.


Class `NodeRect`
- is a subclass of `QGraphicsObject`,
- is the implementation of a viewer/editor of a card.

Class `EdgeArrow`
- inherits `QGraphicsItem`,
- draws an arrow representing a relationship between a pair of cards.
