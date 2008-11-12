CREATE TABLE simcardidmap (
    sqlid INTEGER,
    cardid VARCHAR(255),
    cardindex INTEGER,
    PRIMARY KEY(sqlid),
    UNIQUE(cardid, cardindex));
