graph LR
    subgraph one.cpp
        one_write[Write to STDOUT]
        one_read[Read from STDIN]
    end

    subgraph Coupler
        one_to_parent[one_to_parent pipe]
        parent_to_one[parent_to_one pipe]
        zero_to_parent[zero_to_parent pipe]
        parent_to_zero[parent_to_zero pipe]
        relay[Relay Thread]
    end

    subgraph zero.py
        zero_write[Write to STDOUT]
        zero_read[Read from STDIN]
    end

    one_write -->|writes| one_to_parent
    one_to_parent -->|reads| relay
    relay -->|writes| parent_to_zero
    parent_to_zero -->|reads| zero_read

    zero_write -->|writes| zero_to_parent
    zero_to_parent -->|reads| relay
    relay -->|writes| parent_to_one
    parent_to_one -->|reads| one_read

    classDef process fill:#f9f,stroke:#333,stroke-width:2px;
    classDef pipe fill:#bbf,stroke:#333,stroke-width:2px;
    classDef relay fill:#bfb,stroke:#333,stroke-width:2px;
    
    class one_write,one_read,zero_write,zero_read process;
    class one_to_parent,parent_to_one,zero_to_parent,parent_to_zero pipe;
    class relay relay; 