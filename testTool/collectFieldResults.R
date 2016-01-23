# Collect field results????
library(foreign)
library(EcotopePackage)
library(plyr)

cdx("hpwh")
dirTmp <- "/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/"

sites <- read.dta("../data/site_info.dta")

voltex <- sites[grep("Voltex", sites$make), ]

siteid <- "11531"
readOne <- function(siteid) {
  print(siteid)
  cdx("hpwh")
  dset <- read.dta(file = paste("../data", siteid, "reshaped.dta", sep = "/"))
  dset <- dset[!is.na(dset$Flow) & !is.na(dset$Tintake) & !is.na(dset$Tin), ]
  # dset <- dset[1:220000, ]
  # Need to write out schedules??? "ambient", "draw", "inlet", "evaporator"
  dset <- arrange(dset, readTime)
  dset$minute <- 1:nrow(dset)
  dset$Tintake <- round(dset$Tintake * 1.8 + 32, 1)
  dset$Tin <- round(dset$Tin * 1.8 + 32, 1)
  dset$Tout <- dset$Tout * 1.8 + 32
  
  hourlyFile <- paste("../data", siteid, "hourly.dta", sep = "/")
  hourly <- read.dta(hourlyFile)

  setwd(dirTmp)
  dir2 <- paste0("tests/site", siteid)
  dir.create(dir2)
  dset$time <- stata_time_to_R_time(dset$readTime)
  write.csv(dset[, c("time", "minute", "hpwhW", "compW", "resW")], 
            file = paste(dir2, "fieldResults.csv", sep = "/"),
            row.names = FALSE)
  flowDset <- dset[dset$Flow > 0, c("minute", "Flow")]
  
  # Write out the draw schedule
  drawScheduleFile <- paste(dir2, "drawschedule.csv", sep = "/")
  writeLines(text = "default 0", con = drawScheduleFile)
  write.table(flowDset, file = drawScheduleFile, row.names = FALSE, 
              append = TRUE, sep = ",", quote = FALSE)
  
  # Write out the ambient schedule
  ambientScheduleFile <- paste(dir2, "ambientTschedule.csv", sep = "/")
  writeLines(text = "default 70", con = ambientScheduleFile)
  write.table(dset[, c("minute", "Tintake")], 
              file = ambientScheduleFile, row.names = FALSE, 
              append = TRUE, sep = ",", quote = FALSE)
  
  # Write out the evaporator schedule
  evaporatorScheduleFile <- paste(dir2, "evaporatorTschedule.csv", sep = "/")
  writeLines(text = "default 70", con = evaporatorScheduleFile)
  write.table(dset[, c("minute", "Tintake")], 
              file = evaporatorScheduleFile, row.names = FALSE, 
              append = TRUE, sep = ",", quote = FALSE)
  
  # Write out the inlet schedule
  inletScheduleFile <- paste(dir2, "inletTschedule.csv", sep = "/")
  writeLines(text = "default 55", con = inletScheduleFile)
  write.table(dset[, c("minute", "Tin")], 
              file = inletScheduleFile, row.names = FALSE, 
              append = TRUE, sep = ",", quote = FALSE)  
  
  # Write out the DR schedule
  DRScheduleFile <- paste(dir2, "DRschedule.csv", sep = "/")
  writeLines(text = "default 1\nminutes,OnOff", con = DRScheduleFile)
  
  # What was the setpoint???
  setpoint <- round(as.numeric(quantile(hourly$Tout, .95, na.rm = TRUE)), 1)
  testInfoFile <- paste(dir2, "testInfo.txt", sep = "/")
  writeLines(text = paste0("length_of_test ", max(dset$minute), 
                          "\nsetpoint ", setpoint), 
             con = testInfoFile)

  
}


voltex$siteid
dontUse <- c(23860, 99094)
lapply(voltex$siteid[!(voltex$siteid %in% dontUse)], readOne)

