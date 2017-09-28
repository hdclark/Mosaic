#!/bin/bash

sqlite3 -header -column ~/.Flashcards.db "
    SELECT 
        count(uid) AS 'Number of known cards.'
    FROM sm2 
    ; 

    SELECT 
        count(*) AS 'Number of cards seen at least once.'
    FROM
        ( SELECT DISTINCT uid FROM sm2_responses ) AS r
    ;

    SELECT 
        count(*) AS 'Number of cards seen at least once (alt computation).'
    FROM
        ( SELECT DISTINCT uid FROM sm2 WHERE n_seen > 0 ) AS r
    ;

    SELECT 
        count(*) AS 'Number of cards seen at least twice.'
    FROM
        ( SELECT DISTINCT uid FROM sm2 WHERE n_seen > 1 ) AS r
    ;

    SELECT 
        avg(q) AS 'Average response rate, last 24h.',
        count(q) AS 'Total # of responses, last 24h.'
    FROM 
        sm2_responses 
    WHERE 
        ( strftime('%s', d_t) > strftime('%s', 'now', 'localtime', '-24 hours') )
    ;

    SELECT 
        avg(q) AS 'Average response rate, last 5 days.',
        count(q) AS 'Total # of responses, last 5 days.'
    FROM 
        sm2_responses 
    WHERE 
        ( strftime('%s', d_t) > strftime('%s', 'now', 'localtime', '-5 days') )
    ;

    SELECT 
        avg(q) AS 'Average response rate, last 30 days.',
        count(q) AS 'Total # of responses, last 30 days.'
    FROM 
        sm2_responses 
    WHERE 
        ( strftime('%s', d_t) > strftime('%s', 'now', 'localtime', '-30 days') )
    ;

    SELECT 
        avg(q) AS 'Average response rate, last year.',
        count(q) AS 'Total # of responses, last year.'
    FROM 
        sm2_responses 
    WHERE 
        ( strftime('%s', d_t) > strftime('%s', 'now', 'localtime', '-1 years') )
    ;

  "

