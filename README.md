
# ManiCard

A tool for viewing and editing textual contents and relationships of nodes stored in a Neo4j graph database.

It can be used to organize notes and visualize relations between them, similar to Obsidian's 
Canvas view but backed with a graph database.

Currently it is still in very early phase of development.

## Screenshots

![App Screenshot 1](screenshots/screenshot_1.png)

![App Screenshot 2](screenshots/screenshot_2.png)

## Features

+ Light & dark theme.


## Build & Run the App

Qt 5 and Qt Creator are needed to build the app.

To run the app, you need

+ a self-managed Neo4j DB (Neo4j Aura is not supported)
+ DB setup
    + Create constraints requiring unique value of `id` property for nodes with label `Card`, `CustomDataQuery`, `GroupBox`, `Board`,  and `Workspace`.
    ```cypher
    CREATE CONSTRAINT unique_card_id
    FOR (c:Card) REQUIRE c.id IS UNIQUE;
    ```
    ```cypher
    CREATE CONSTRAINT unique_custom_data_query_id
    FOR (q:CustomDataQuery) REQUIRE q.id IS UNIQUE;
    ```
    ```cypher
    CREATE CONSTRAINT unique_group_box_id
    FOR (q:GroupBox) REQUIRE q.id IS UNIQUE;
    ```
    ```cypher
    CREATE CONSTRAINT unique_board_id
    FOR (b:Board) REQUIRE b.id IS UNIQUE;
    ```
    ```cypher
    CREATE CONSTRAINT unique_workspace_id
    FOR (w:Workspace) REQUIRE w.id IS UNIQUE;
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

+ Only supports self-managed Neo4j DB (as opposed to Neo4j Aura).
+ Only supports single-instance Neo4j DB (as opposed to a cluster).
+ No automatic sync between multiple instances of the app (running on difference devices) that connect to the 
  same Neo4j DB instance. It is recommended to run the app on single device at a time.
+ Updates that cannot be saved to DB because of connection issues are recorded in a text file, but they are not 
  retried. After resolving DB connection issues, you need to re-open the app, and apply the unsaved 
  updates manually. It is recommended to install the Neo4j DB locally (i.e., on the device where the app is running).
