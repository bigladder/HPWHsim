library(ggplot2)
library(EcotopePackage)
library(foreign)

setwd("/storage/RBSA_secure/HPWHsim/Condensity/")
dir()

plot_one <- function(test) {
  model <- gsub("^.+_(.+)_.+$", "\\1", test)
  fname <- paste("models", model, test, "hpwhTestToolOutput.csv", sep = "/")
  dset <- read.csv(fname)
  
  names(dset)[names(dset) == "minute_of_year"] <- "minutes"
  dlong <- reshape2::melt(dset, id.vars = c("minutes"))
  
  dlong$Tcouple <- as.numeric(gsub("^T\\.([0-9]+)\\.", "\\1", dlong$variable))
  dlong$sim <- "Simulated"
  
  wd <- getwd()
  cdx("hpwhlab")
  if(length(grep("Voltex60", test))) {
    test2 <- gsub("Voltex60", "AOSmith60", test)  
  } else if(length(grep("ATI66", test))) {
    test2 <- gsub("ATI66", "ATI66rev2", test)    
  } else {
    test2 <- test
  }

  lab <- read.dta(paste("../data/prepped/", test2, ".dta", sep = ""))
  tcouples <- grep("T_Tank_[0-9]", names(lab))
  minutes <- which(names(lab) == "minutes")
  lab <- lab[, c(minutes, tcouples)]
  lablong <- reshape2::melt(lab, id.vars = "minutes")
  lablong$sim <- "Measured"
  lablong$Tcouple <- as.numeric(gsub("^T_Tank_([0-9]+)$", "\\1", lablong$variable))
  lablong <- lablong[lablong$Tcouple %in% c(2, 4, 6, 8, 10, 12), ]
  lablong$value <- lablong$value * 1.8 + 32
  
  along <- rbind(dlong, lablong)
  
  setwd(wd)
  
  ggplot(along[!is.na(along$Tcouple) & along$minutes < 600, ]) + 
    theme_bw() +
    geom_line(aes(minutes, value, group = interaction(Tcouple, sim), col = sim), alpha = .6) +
    facet_wrap(~sim, nrow = 2)
  
}

setwd("/storage/RBSA_secure/HPWHsim/Condensity/")
DOE_Voltex60_24hr67 <- plot_one("DOE_Voltex60_24hr67") +
  ggtitle("Voltex 60 Gallon DOE EF test Thermocouples") +
  xlab("Minutes Into Test") +
  ylab("Temperature (F)")
DOE_Voltex60_24hr67
ggsave(file = "DOE_Voltex60_24hr67.png", DOE_Voltex60_24hr67, width = 7, height = 5)


DOE_Voltex60_24hr50 <- plot_one("DOE_Voltex60_24hr50") +
  ggtitle("Voltex 60 Gallon DOE 50F EF test Thermocouples") +
  xlab("Minutes Into Test") +
  ylab("Temperature (F)")
DOE_Voltex60_24hr50
ggsave(file = "DOE_Voltex60_24hr50.png", DOE_Voltex60_24hr50, width = 7, height = 5)


DOE_ATI66_24hr67 <- plot_one("DOE_ATI66_24hr67") +
  ggtitle("ATI 66 Gallon DOE EF test Thermocouples") +
  xlab("Minutes Into Test") +
  ylab("Temperature (F)")
DOE_ATI66_24hr67
ggsave(file = "DOE_ATI66_24hr67.png", DOE_ATI66_24hr67, width = 7, height = 5)

plot_one("DOE_ATI66_24hr50")


DOE_GEred_24hr67 <- plot_one("DOE_GEred_24hr67")+
  ggtitle("GE Red DOE EF test Thermocouples") +
  xlab("Minutes Into Test") +
  ylab("Temperature (F)")
DOE_GEred_24hr67
ggsave(file = "DOE_GEred_24hr67.png", DOE_GEred_24hr67, width = 7, height = 5)



# Have to manually set to Stiebel in the code to make this fake thing work...
DOE_Stiebel_24hr67 <- plot_one("DOE_Stiebel_24hr67") +
  ggtitle("ATI 66 Gallon DOE EF test Thermocouples") +
  xlab("Minutes Into Test") +
  ylab("Temperature (F)")
DOE_Stiebel_24hr67


