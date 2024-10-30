# include docker-compose .env file
include .env

.PHONY: bts hlr mgw stp msc bsc trx base

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


all: base bts hlr mgw stp msc bsc trx

clean:
	docker image remove base-open5gs:${OSMOBTS_VERSION} amf:${OSMOBTS_VERSION} ausf:${OSMOBTS_VERSION} bsf:${OSMOBTS_VERSION} nrf:${OSMOBTS_VERSION} nssf:${OSMOBTS_VERSION} \
	pcf:${OSMOBTS_VERSION} scp:${OSMOBTS_VERSION} sepp:${OSMOBTS_VERSION} smf:${OSMOBTS_VERSION} udm:${OSMOBTS_VERSION} udr:${OSMOBTS_VERSION} upf:${OSMOBTS_VERSION} \
	webui:${OSMOBTS_VERSION}
