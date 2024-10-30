# include docker-compose .env file
include .env

.PHONY: bts hlr mgw stp msc bsc trx base

ggsn:
	docker build -t osmo-ggsn:${OSMOBTS_VERSION} ./images/ggsn

sgsn: hlr
	docker build -t osmo-sgsn:${OSMOBTS_VERSION} ./images/sgsn

pcu:
	docker build -t osmo-pcu:${OSMOBTS_VERSION} ./images/pcu

bts:
	docker build -t osmo-bts:${OSMOBTS_VERSION} ./images/bts

hlr:
	docker build -t osmo-hlr:${OSMOBTS_VERSION} ./images/hlr

mgw:
	docker build -t osmo-mgw:${OSMOBTS_VERSION} ./images/mgw

stp:
	docker build -t osmo-stp:${OSMOBTS_VERSION} ./images/stp

msc: stp
	docker build -t osmo-msc:${OSMOBTS_VERSION} ./images/msc

bsc: msc
	docker build -t osmo-bsc:${OSMOBTS_VERSION} ./images/bsc

trx:
	docker build -t osmo-trx:${OSMOBTS_VERSION} ./images/trx

base:
	docker build -t osmo-base:${OSMOBTS_VERSION} ./images/base


all: base bts hlr mgw stp msc bsc trx sgsn pcu ggsn

clean:
	docker image remove osmo-base:${OSMOBTS_VERSION} osmo-bts:${OSMOBTS_VERSION} osmo-hlr:${OSMOBTS_VERSION} \ 
		osmo-mgw:${OSMOBTS_VERSION} osmo-stp:${OSMOBTS_VERSION} osmo-stp:${OSMOBTS_VERSION} \
		osmo-msc:${OSMOBTS_VERSION} osmo-bsc:${OSMOBTS_VERSION} osmo-trx:${OSMOBTS_VERSION} osmo-trx:${OSMOBTS_VERSION} \ 
		osmo-sgsn:${OSMOBTS_VERSION} osmo-pcu:${OSMOBTS_VERSION} osmo-ggsn:${OSMOBTS_VERSION}
