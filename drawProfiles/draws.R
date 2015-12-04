library(ggplot2)
library(EcotopePackage)
library(foreign)
library(lubridate)

setwd("/storage/server/SEEM/SEEM97/")

drawsWeekly <- read.csv("Draw_schedule_by_occupancy_weekly.csv")
drawsDaily <- read.csv("Draw_schedule_by_occupancy.csv")

drawsWeekly$day <- rep(1:7, each = 60 * 24)
drawsWeekly$hour <- rep(0:23, 7, each = 60)
drawsDaily$hour <- rep(0:23, each = 60)

drawsWeekly$hour2 <- (drawsWeekly$time / 60) %% 24

setwd("/storage/RBSA_secure/HPWHsim/")
occ1Plot <- ggplot(drawsWeekly) + theme_bw() + 
  geom_line(aes(hour2, Flow_2, group = day)) +
  scale_x_continuous(breaks = seq(0, 24, 2)) +
  ggtitle("All Weekly Draw Profile Draws - 1 Occupant") +
  xlab("Hour of Day") + ylab("GPM")
occ1Plot
ggsave(file = "occ1Plot.png", occ1Plot, width = 7, height = 5)


drawsWeekly <- reshape2::melt(drawsWeekly, id.vars = c("time", "day", "hour"))
drawsWeekly$type <- "Weekly"
drawsDaily <- reshape2::melt(drawsDaily, id.vars = c("time", "hour"))
drawsDaily$type <- "Daily"

drawsWeekly <- aggregate(value ~ hour + variable + type, 
                         data = drawsWeekly, FUN = sum)
drawsWeekly$value <- drawsWeekly$value / 7
drawsDaily <- aggregate(value ~ hour + variable + type,
                        data = drawsDaily, FUN = sum)

draws <- rbind(drawsDaily, drawsWeekly)

hourBreaks <- seq(0, 22, 2)
hourLabels <- c("Midnight", "2AM", "4AM", "6AM", "8AM", "10AM", "Noon", "2PM", "4PM", "6PM", "8PM", "10PM")
ggplot(draws) + theme_bw() + 
  geom_line(aes(hour, value, col = type), size = 2) +
  facet_wrap(~ variable, ncol = 2) +
  ggtitle("HPWHsim Draw Profiles") +
  xlab("Hour of Day") + ylab("Gallons per Hour") +
  theme(axis.text.x = element_text(angle = 45, hjust = 1)) +
  scale_x_continuous(breaks = hourBreaks, labels = hourLabels)




# Need to get measured data...
cdx("hpwh")
setwd("../analysis/code")

source("EnrichFunctions.R")


site_info <- get_site_info()

dset <- grab_data(site_info, codes = "Flow", sites = "All", agg = "hourly")
dset$hour <- hour(dset$time)
dset2 <- aggregate(Flow ~ hour + siteid + total_occ, data = dset, FUN = mean)
rm(dset)


dset2$variable <- paste("Flow", dset2$total_occ, sep = "_")
dset2$variable[dset2$total_occ > 5] <- "Flow_5"

dset3 <- aggregate(Flow ~ hour + variable, data = dset2, FUN = mean)
# dset3 <- reshape2::melt(dset3, id.vars = "hour")
names(dset3)[names(dset3) == "Flow"] <- "value"
dset3$type <- "Measured"
head(dset3)

ggplot(dset2) + theme_bw() + 
  geom_line(aes(hour, Flow, group = siteid)) + 
  facet_wrap(~total_occ) +
  ggtitle("Hourly Average Draw Profiles by Number of Occupants") +
  xlab("Hour of Day") + ylab("Average Hourly Gallons of Hot Water")

draws <- rbind(draws, dset3)
sizes <- data.frame("type" = c("Daily", "Weekly", "Measured"),
                    "type2" = c("Daily Draw Profile", "Weekly Draw Profile", "Average Measured"),
                    "sizex" = c(1, 2, 3))
dim(draws)
draws <- merge(draws, sizes)
draws$type2 <- factor(draws$type2, levels = c("Daily Draw Profile", "Weekly Draw Profile", "Average Measured"))

boxes <- data.frame("variable" = c("Flow_1", "Flow_2", "Flow_3", "Flow_4", "Flow_5"),
                    "variable2" = c("1 Occupant", "2 Occupants", "3 Occupants", "4 Occupants", "5+ Occupants"))
draws <- merge(draws, boxes)

setwd("/storage/RBSA_secure/HPWHsim/")
compPlot <- ggplot(draws) + theme_bw() + 
  geom_line(aes(hour, value, col = type2, size = type2)) +
  facet_wrap(~ variable2, ncol = 2) +
  ggtitle("HPWHsim Draw Profiles") +
  xlab("Hour of Day") + ylab("Gallons per Hour") +
  theme(axis.text.x = element_text(angle = 45, hjust = 1)) +
  scale_x_continuous(breaks = hourBreaks, labels = hourLabels) +
  scale_size_discrete(name = "", range = c(.3, 1.8)) +
  scale_colour_discrete(name = "")
compPlot
ggsave(file = "compPlot.png", compPlot, width = 7, height = 5)


draws$hour2 <- draws$hour
draws$hour2[draws$type == "Weekly"] <- draws$hour[draws$type == "Weekly"] + 2
draws$value2 <- draws$value
draws$value2[draws$type == "Weekly" & draws$hour2 > 23] <- NA

stupid <- data.frame("variable" = "abc",
                     "type" = "Weekly",
                     "hour" = 0,
                     "value" = 0,
                     "type2" = "Weekly Draw Profile",
                     "sizex" = 2,
                     "variable2" = c("1 Occupant", "2 Occupants", "3 Occupants", "4 Occupants", "5+ Occupants"),
                     "hour2" = 0,
                     "value2" = 0)
drawsx <- rbind(draws, stupid)

compPlotCorrected <- ggplot(drawsx) + theme_bw() + 
  geom_line(aes(hour2, value2, col = type2, size = type2)) +
  facet_wrap(~ variable2, ncol = 2) +
  ggtitle("HPWHsim Draw Profiles") +
  xlab("Hour of Day") + ylab("Gallons per Hour") +
  theme(axis.text.x = element_text(angle = 45, hjust = 1)) +
  scale_x_continuous(breaks = hourBreaks, labels = hourLabels) +
  scale_size_discrete(name = "", range = c(.3, 1.8)) +
  scale_colour_discrete(name = "")
compPlotCorrected
ggsave(file = "compPlotCorrected.png", compPlotCorrected, width = 7, height = 5)



nSites <- aggregate(siteid ~ total_occ, data = dset2, FUN = function(x) {
  length(unique(x))
})
nSites$n <- paste(nSites$total_occ, "occupants:", nSites$siteid, "sites")
nSites$n[1] <- "1 occupant: 14 sites"
nSites$n[7] <- "7 occupants: 1 site"
nSites$n[8] <- "8 occupants: 1 site"

dset2 <- merge(dset2, nSites[, c("total_occ", "n")])

mPlot <- ggplot(dset2) + theme_bw() + 
  geom_line(aes(hour, Flow, group = siteid), size = .2) + 
  facet_wrap(~n) +
  ggtitle("Hourly Average Draw Profiles by Number of Occupants") +
  xlab("Hour of Day") + ylab("Average Hourly Gallons of Hot Water")
mPlot
ggsave(file = "mPlot.png", mPlot, width = 7, height = 5)
