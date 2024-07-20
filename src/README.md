
# ManiCard

A simple tool for viewing and editing textual properties and relationships of nodes with 
label `Card` in Neo4j graph database.

It can be used to organize notes and visualize relations between them, similar to Obsidian's 
Canvas view but backed with a graph database.

Currently still in very early phase of development.

## Screenshots

![App Screenshot](https://firebasestorage.googleapis.com/v0/b/personal-shared-files.appspot.com/o/public%2FManiCard_ExampleBoard_2.png?alt=media)


## Build & Run the App

Qt and Qt creator are needed to build the app.

To run the app, you need

+ a self-managed Neo4j DB (currently Neo4j Aura does not support HTTP API)
+ DB setup
    + Create constraint requiring unique value of `id` property for nodes with label `Card`, and 
      similarly for nodes of label `Board`.
        ```cypher
        CREATE CONSTRAINT unique_card_id
        FOR (c:Card) REQUIRE c.id IS UNIQUE;
        ```
        ```cypher
        CREATE CONSTRAINT unique_board_id
        FOR (b:Board) REQUIRE b.id IS UNIQUE;
        ```
    + Create nodes for last-used IDs for cards and boards.
        ```cypher
        MERGE (n:LastUsedId {itemType: 'Card'})
        ON CREATE SET n.value = 0
        RETURN n;
        ```   
        ```cypher
        MERGE (n:LastUsedId {itemType: 'Board'})
        ON CREATE SET n.value = 0
        RETURN n;
        ```
+ a `config.json` file put under the same directory as the executable (see `src/config.example.json` 
  for an example)

### Current Limitations

+ Only supports single-instance Neo4j DB (as apposed to a cluster).
+ No automatic sync among multiple instances of the app (on difference devices) connecting to the 
  same Neo4j DB instance. You need to manually reload the data.
+ Unsaved updates because of DB connection issues are recorded in a text file, but they are not 
  retried. After resolving DB connection issues, you need to re-open the app, and apply the unsaved 
  updateds manually.
+ No markdown preview (yet).
