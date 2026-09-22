import CyberTexel

do {
  _ = try Document()
} catch {
  fatalError("CyberTexel link check failed: \(error)")
}
