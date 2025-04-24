```mermaid
usecase
  actor Pilot
  actor Instructor
  actor "Regulatory Body" as Regulator
  actor "Hardware Vendor" as Vendor

  Pilot --> (Select Training Scenario)
  Instructor --> (Select Training Scenario)
  Pilot --> (Fly Aircraft)
  Pilot --> (Manage Flight Plan)
  Pilot --> (Handle Failures)
  Pilot --> (View Feedback)
  Instructor --> (View Feedback)
  Pilot --> (Configure Hardware)
  Regulator --> (Generate Compliance Report)
  Vendor --> (Configure Hardware)

  (Fly Aircraft) ..> (Manage Flight Plan) : include
  (Fly Aircraft) ..> (Handle Failures) : extend
  (Fly Aircraft) ..> (View Feedback) : include

  note right of (Fly Aircraft)
    Includes real-time interaction with
    flight dynamics and avionics
  end note

  note right of (Generate Compliance Report)
    Required for FAA/EASA audits
  end note
```
