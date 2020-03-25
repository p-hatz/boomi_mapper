CREATE TABLE component (
    id    INTEGER       PRIMARY KEY,
    uid   VARCHAR (64),
    name  VARCHAR (500),
    _type VARCHAR (100),
    cr_dt DATETIME      DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE imp_component (
    id    INTEGER       PRIMARY KEY,
    uid   VARCHAR (64),
    name  VARCHAR (500),
    _type VARCHAR (100),
    p_uid VARCHAR (64),
    r_uid VARCHAR (64),
    cr_dt DATETIME      DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE imp_process (
    id    INTEGER       PRIMARY KEY,
    uid   VARCHAR (64),
    name  VARCHAR (500),
    cr_dt DATETIME      DEFAULT CURRENT_TIMESTAMP
);


CREATE TABLE process (
    id    INTEGER       PRIMARY KEY,
    uid   VARCHAR (64),
    name  VARCHAR (500),
    cr_dt DATETIME      DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE source_node (
    id    INTEGER      PRIMARY KEY,
    uid   VARCHAR (64),
    r_uid VARCHAR (64),
    cr_dt DATETIME     DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE source_node_component (
    node_id      INTEGER,
    component_id INTEGER,
    cr_dt        DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (
        node_id
    )
    REFERENCES source_node (id),
    FOREIGN KEY (
        component_id
    )
    REFERENCES component (id) 
);

create view vw_edges
as
select c_s.id source, c_t.id target,s_n.r_uid--,p.name  
  from "source_node_component" n_c inner join "source_node" s_n
  on n_c.node_id = s_n.id
  inner join component c_s
  on s_n.uid = c_s.uid
  inner join component c_t
  on n_c.component_id = c_t.id
  inner join process p
  on s_n.r_uid = p.uid
  where c_s.id <> c_t.id;

create view vw_get_process_component_map
as
select p.name Main_Process,p.uid,c_s.name Source_Node,c_s.id source, c_s._type source_type, c_t.name Target_Node, c_t.id target, c_t._type target_type
  from process p inner join vw_edges v_e
  on p.uid = v_e.r_uid
  inner join component c_s
  on v_e.source = c_s.id
  inner join component c_t
  on v_e.target = c_t.id;

create view vw_get_source_component_map
as
select p.name Main_Process,p.uid,c_t.name Source_Node,c_t.id source, c_t._type source_type, c_s.name Target_Node, c_s.id target, c_s._type target_type
  from process p inner join vw_edges v_e
  on p.uid = v_e.r_uid
  inner join component c_s
  on v_e.source = c_s.id
  inner join component c_t
  on v_e.target = c_t.id;

create view vw_nodes
as
select id,name as label from component c;

create view vw_nodes_source
as
select source id,source_node || ' (' || main_process || ')' label
 from vw_get_source_component_map
 union
  select target,Target_Node || ' (' || main_process || ')'
  from  vw_get_source_component_map;