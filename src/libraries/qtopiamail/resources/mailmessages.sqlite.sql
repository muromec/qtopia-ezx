CREATE TABLE mailmessages ( 
    id INTEGER PRIMARY KEY,
    type INTEGER NOT NULL,
    parentfolderid INTEGER NOT NULL,
    sender VARCHAR,
    recipients VARCHAR,
    subject VARCHAR,
    stamp TIMESTAMP,
    status INTEGER,
    fromaccount VARCHAR,
    frommailbox VARCHAR,
    mailfile VARCHAR,
    serveruid VARCHAR,
    size INTEGER,
    FOREIGN KEY (parentfolderid) REFERENCES mailfolders(id));

CREATE INDEX parentfolderid_idx ON mailmessages("parentfolderid");  
CREATE INDEX fromaccount_idx ON mailmessages("fromaccount");
CREATE INDEX frommailbox_idx ON mailmessages("frommailbox");
CREATE INDEX stamp_idx ON mailmessages("stamp");
